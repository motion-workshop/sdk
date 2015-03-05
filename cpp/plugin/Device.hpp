/*
  @file    tools/sdk/cpp/plugin/Device.hpp
  @author  Luke Tokheim, luke@motionnode.com
  @version 2.0

  (C) Copyright Motion Workshop 2013. All rights reserved.

  The coded instructions, statements, computer programs, and/or related
  material (collectively the "Data") in these files contain unpublished
  information proprietary to Motion Workshop, which is protected by
  US federal copyright law and by international treaties.

  The Data may not be disclosed or distributed to third parties, in whole
  or in part, without the prior written consent of Motion Workshop.

  The Data is provided "as is" without express or implied warranty, and
  with no claim as to its suitability for any purpose.
*/
#ifndef __MOTION_SDK_PLUGIN_DEVICE_HPP_
#define __MOTION_SDK_PLUGIN_DEVICE_HPP_

#include <map>
#include <queue>
#include <string>
#include <vector>

/**
  Depends on the Boost C++ libraries, available at http://www.boost.org/.
  Requires compilation of the Thread and System libraries.
*/
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include <Client.hpp>
#include <Format.hpp>


namespace Motion { namespace SDK { namespace Device {

/**
  Utility container. Bool, Mutex, and Condition variable. Super combo.
*/
template <
  typename Mutex,
  typename Lock,
  typename Condition
>
class blocking_bool {
 public:
  typedef boost::logic::tribool value_type;

  /** Constructor. Put this container into the indeterminate state. */
  explicit blocking_bool()
    : m_state(new state())
  {
  }

  /**
    Read accessor, as a static cast operator.
 
    @return Returns true iff internal value is true.
  */
  operator bool() const
  {
    lock_type lock(m_state->mutex);
    return (m_state->value == boost::logic::tribool(true));
  }

  /**
    Blocking equality operator.

    @post   This container is not in the indeterminate state.
  */
  bool operator<<(bool rhs) const
  {
    lock_type lock(m_state->mutex);
    if (boost::logic::indeterminate(m_state->value)) {
      if (m_state->time_out > 0) {
       boost::xtime timestamp;
        {
          // boost::TIME_UTC_
          boost::xtime_get(&timestamp, 1);
          timestamp.sec += m_state->time_out;
        }

        m_state->condition.timed_wait(lock, timestamp);
      } else {
        m_state->condition.wait(lock);
      }
    }

    return (m_state->value == boost::logic::tribool(rhs));
  }

  /**
    Write accessor, as a simple assignment operator.

    @param  value Copy the input into the internal state value.
    @post   This container is not in the indeterminate state.
  */
  void operator=(bool rhs)
  {
    {
      lock_type lock(m_state->mutex);
      if (rhs) {
        m_state->value.value = boost::logic::tribool::true_value;
      } else {
        m_state->value.value = boost::logic::tribool::false_value;
      }
    }
    m_state->condition.notify_all();
  }

  /** Put this combo into the indeterminate state. */
  void indeterminate()
  {
    lock_type lock(m_state->mutex);
    m_state->value.value = boost::logic::tribool::indeterminate_value;
  }

  void time_out(const std::size_t &second)
  {
    lock_type lock(m_state->mutex);
    m_state->time_out = static_cast<time_out_type>(second);
  }

 private:
  typedef Mutex mutex_type;
  typedef Lock lock_type;
  typedef Condition condition_type;
  typedef std::size_t time_out_type;

  /** Simple data container for all internal state. */
  class state {
   public:
    state()
      : value(boost::logic::indeterminate),
        mutex(), condition(), time_out()
    {
    }

    value_type value;
    mutex_type mutex;
    condition_type condition;
    time_out_type time_out;
  }; // class state

  /** Shared pointer to state. Copyable thread safe shared value. */
  boost::shared_ptr<state> m_state;
}; // class blocking_bool

/**
  Thread safe (mutex protected) shared state container for Motion connections.
*/
template <
  typename Mutex,
  typename Lock
>
class State {
 public:
  State()
    : m_mutex(new Mutex()),
      m_state(new state_type(false, false, false, std::string()))
  {
  }

  bool quit() const
  {
    Lock lock(*m_mutex);
    return m_state->get<Quit>();
  }

  bool connected() const
  {
    Lock lock(*m_mutex);
    return m_state->get<Connected>();
  }

  bool reading() const
  {
    Lock lock(*m_mutex);
    return m_state->get<Reading>();
  }

  std::string xml_string() const
  {
    Lock lock(*m_mutex);
    return m_state->get<XMLString>();
  }

 private:
  enum {
    Quit = 0,
    Connected,
    Reading,
    XMLString
  };

  typedef boost::tuple<bool,bool,bool,std::string> state_type;

  boost::shared_ptr<Mutex> m_mutex;
  boost::shared_ptr<state_type> m_state;

  void quit(bool value)
  {
    Lock lock(*m_mutex);
    m_state->get<Quit>() = value;
  }

  void connected(bool value)
  {
    Lock lock(*m_mutex);
    m_state->get<Connected>() = value;
  }

  void reading(bool value)
  {
    Lock lock(*m_mutex);
    m_state->get<Reading>() = value;
  }

  void xml_string(const std::string &value)
  {
    Lock lock(*m_mutex);
    m_state->get<XMLString>() = value;
  }

  template <
    typename DataFunctionT,
    typename MutexT,
    typename LockT,
    typename ConditionT
  >
  friend class Reader;

  template <
    typename SamplerT,
    typename ThreadT,
    typename MutexT,
    typename LockT,
    typename ConditionT
  >
  friend class Manager;
}; // class State


/**
  A Sampler is intended to provide simple and flexible access the real-time data
  streams from the Motion system. A Sampler instance is shared between the
  client application (the "main") and an asynchronous communication handler
  (the "thread"). The client application polls the Sampler instance at its own
  interval, independent of the rate of the associated data stream.
*/
template <
  typename Mutex,
  typename ScopedLock,
  typename Condition,
  typename Data=Format::preview_service_type
>
class Sampler {
public:
  typedef Data data_type;
  typedef boost::function<bool ()> function_type;
  typedef Mutex mutex_type;
  typedef ScopedLock scoped_lock_type;
  typedef Condition condition_type;

  Sampler(const std::string &address, const std::size_t &port,
          const std::string &initialize=std::string(),
          const function_type &callback=function_type())
    : m_address(address), m_port(port), m_initialize(initialize), m_key(),
      m_sampler_id(),
#if MOTION_DEVICE_BUFFERED
      m_list_max(new std::size_t()), m_list(new list_type()),
#else
      m_data(new data_type()),
#endif  // MOTION_DEVICE_BUFFERED
      m_mutex(new mutex_type()), m_condition(new condition_type()),
      m_callback(callback)
  {
  }

  virtual ~Sampler()
  {
  }

  virtual bool get_data(Data &data)
  {
    bool result = false;

    data.clear();
    {
      ScopedLock lock(*m_mutex);
#if MOTION_DEVICE_BUFFERED
      if (!m_list->empty() && !m_list->front().empty()) {
        data = m_list->front();
        m_list->pop();
        result = true;
      }
#else
      if (!m_data->empty()) {
        data = *m_data;
        result = true;
      }
#endif // MOTION_DEVICE_BUFFERED
    }

    return result;
  }

  virtual bool get_data_block(Data &data)
  {
    bool result = false;

    data.clear();
    {
      ScopedLock lock(*m_mutex);
#if MOTION_DEVICE_BUFFERED
      if (m_list->empty()) {
        m_condition->wait(lock);
      }

      if (!m_list->empty() && !m_list->front().empty()) {
        data = m_list->front();
        m_list->pop();
        result = true;
      }
#else
      m_condition->wait(lock);
      if (!m_data->empty()) {
        data = *m_data;
        result = true;
      }
#endif // MOTION_DEVICE_BUFFERED
    }

    return result;
  }

  virtual bool get_data_block(Data &data, const std::size_t &time_out_second)
  {
    bool result = false;

    data.clear();
    {
      boost::xtime timestamp;
      {
#if BOOST_VERSION < 105000
        boost::xtime_get(&timestamp, boost::TIME_UTC);
#else
        boost::xtime_get(&timestamp, boost::TIME_UTC_);
#endif // BOOST_VERSION
        timestamp.sec += time_out_second;
      }

      ScopedLock lock(*m_mutex);
#if MOTION_DEVICE_BUFFERED
      if (m_list->empty()) {
        m_condition->timed_wait(lock, timestamp);
      }

      if (!m_list->empty() && !m_list->front().empty()) {
        data = m_list->front();
        m_list->pop();
        result = true;
      }
#else
      if (m_condition->timed_wait(lock, timestamp)) {
        if (!m_data->empty()) {
          data = *m_data;
          result = true;
        }
      }
#endif  // MOTION_DEVICE_BUFFERED
    }

    return result;
  }

  bool is_connected() const
  {
    return m_state.connected();
  }
  
  bool is_reading() const
  {
    return m_state.reading();
  }

  bool is_quit() const
  {
    return m_state.quit();
  }

  std::string get_xml_string() const
  {
    return m_state.xml_string();
  }

  bool set_list_maximum(const std::size_t &value)
  {
#if MOTION_DEVICE_BUFFERED
    {
      ScopedLock lock(*m_mutex);
      *m_list_max = value;
    }
    return true;
#else
    return false;
#endif // MOTION_DEVICE_BUFFERED
  }

  std::size_t get_list_size() const
  {
#if MOTION_DEVICE_BUFFERED
    {
      ScopedLock lock(*m_mutex);
      return m_list->size();
    }
#else
    return 0;
#endif // MOTION_DEVICE_BUFFERED
  }

 protected:
  /**
    Called by communication thread this sampler is currently attached to.

    This method is virtual just in case we want to override the default
    functionality. This implementation allows for both polling threads and in
    thread callbacks so hopefully that will cover everyone.
  */
  virtual bool set_data(const Data &data)
  {
    bool result = false;

    // Copy the incoming data into the buffer local to this instance.
    {
      ScopedLock lock(*m_mutex);
      if (std::size_t() == m_key) {
        // Wholesale buffer copy.
#if MOTION_DEVICE_BUFFERED
        m_list->push(data);
#else
        *m_data = data;
#endif  // MOTION_DEVICE_BUFFERED
        result = true;
      } else {
        // Filter by key. Actually, just look up the one allowed key and copy
        // that element.
        Data filtered_data;
        {
          typename Data::const_iterator itr = data.find(m_key);
          if (data.end() != itr) {
            filtered_data.insert(*itr);
          }
        }

        if (!filtered_data.empty()) {
#if MOTION_DEVICE_BUFFERED
          m_list->push(data);
#else
          *m_data = data;
#endif  // MOTION_DEVICE_BUFFERED
          result = true;
        }
      }

#if MOTION_DEVICE_BUFFERED
      if (*m_list_max > 0) {
        while (m_list->size() > *m_list_max) {
          m_list->pop();

          if (*m_list_max > 1) {
            // This should be user defined behavior. If we
            // are in buffered mode and the buffer fills up
            // past our requested limit is it an error or do
            // we simply throw away old samples.
            // For now, set an error condition if the user
            // requested a buffer size of 2 or greater.
            result = false;
          }
        }
      }
#endif  // MOTION_DEVICE_BUFFERED
    }

    // Notify the listening class in this
    // thread that a new sample has arrived.
    // uses a regular old fashioned callback.
    if (m_callback) {
      m_callback();
    }

    // Notify other polling threads that may
    // be blocking in get_data_block() that
    // a new sample has arrived.
    m_condition->notify_all();

    return result;
  }

 private:
  /**
    Queue of data. Save multiple samples since we
    may receive more than one between calls to
    read_data.
  */
  typedef typename std::queue<Data> list_type;

  /** Remote IP address of the data service. */
  std::string m_address;

  /** Remote port number of the data service. */
  std::size_t m_port;

  /**
    Initialize the data service by send this string at
    connect time
  */
  std::string m_initialize;

  /** Identifier of the node data to return. */
  std::size_t m_key;

  /** For internal usage only. Unique id for the Manager. */
  std::size_t m_sampler_id;

#if MOTION_DEVICE_BUFFERED
  /**
    Maximum size of the sample queue. Defaults to a single
    sample. Set to zero for unlimited queue size.
  */
  boost::shared_ptr<std::size_t> m_list_max;

  /**
    FIFO queue of samples.
  */
  boost::shared_ptr<list_type> m_list;
#else
  /**
    Internal data buffer. Each sampler instance has
    its own copy of the data.
  */
  boost::shared_ptr<Data> m_data;
#endif  // MOTION_DEVICE_BUFFERED

  /** Protects the queue of data. */
  boost::shared_ptr<Mutex> m_mutex;

  /** Implement blocking reads on data queue. */
  boost::shared_ptr<Condition> m_condition;

  /**
    Optional callback function for data propagation into some external storage
    in the communication thread.
  */
  boost::function<void ()> m_callback;

  State<Mutex,ScopedLock> m_state;

  template <
    typename SamplerT,
    typename ThreadT,
    typename MutexT,
    typename LockT,
    typename ConditionT
  >
  friend class Manager;
}; // class Sampler


/**
  Implements I/O thread for a single connection to a Motion Service data stream.
*/
template <
  typename DataFunction,
  typename Mutex,
  typename Lock,
  typename Condition
>
class Reader : private boost::noncopyable {
public:
  Reader(const std::string &address, const std::size_t &port,
         const std::string &initialize, DataFunction data_fn=DataFunction())
    : m_address(address), m_port(port), m_initialize(initialize),
      m_data_fn(data_fn), m_state(), m_running()
  {
  }

  void operator()()
  {
    try {
      // Initialize book keeping flags.
      m_state.connected(false);
      m_state.reading(false);


      // Open a connection to the Motion Service Preview stream running
      // on Host:Port.
      Client client(m_address, static_cast<unsigned>(m_port));

      // Enter the connected state.
      m_state.connected(true);

      // Send initialization string to the data service if
      // we have one.
      if (!m_initialize.empty()) {
        Client::data_type data(m_initialize.begin(), m_initialize.end());
        if (client.writeData(data)) {
          // Success. But no need for feedback on this one.
        }
      }

      // We have opened up a connection. Enter the thread loop and assume that
      // we have successfully started running this thread.
      m_running = true;

      std::string xml_string;
      while (!m_state.quit()) {

        // Block on this call until a single data sample arrives from the remote
        // service, timing out after the default time period (5 seconds).
        if (client.waitForData() && !m_state.quit()) {

          // Enter the reading state. 
          m_state.reading(true);

          // The Client#readData method will time out after 1 second. Then just
          // go back to the blocking Client#waitForData method to wait for more
          // incoming data.
          Client::data_type data;
          while (client.readData(data) && !m_state.quit()) {
            // Track the most recent XML message. Export
            // changes to the shared state.
            std::string new_string;
            if (client.getXMLString(new_string) && (new_string != xml_string)) {
              xml_string = new_string;
              m_state.xml_string(xml_string);
            }

            // Export this data sample to all attached sampler
            // objects.
            if (m_data_fn) {
              if (!m_data_fn(data)) {
                m_state.quit(true);
                break;
              }
            }
          }

          // Leave the reading state.
          m_state.reading(false);
        }
      }

#if MOTION_SDK_USE_EXCEPTIONS
    } catch (detail::error &) {
#endif  // MOTION_SDK_USE_EXCEPTIONS
    } catch (...) {
    }

    // Initialize book keeping flags. Only when the Client object goes out of
    // scope do we say that we are no longer connected.
    {
      m_state.connected(false);
      m_state.reading(false);

      m_state.quit(true);
    }

    // Send a signal that we are no longer connected. This will propagate
    // through the regular pipeline and notify any Sampler objects that we are
    // going to quit.
    if (m_data_fn) {
      m_data_fn(Client::data_type());
    }

    // Note, if the main thread is blocking on the running flag we cannot call
    // the data callback. We will enter sweet enternal deadlock. Set the running
    // flag to a determined value so that running will unblock.
    m_running = false;
  }

  void quit(bool value)
  {
    m_state.quit(value);
  }

  void set_data_fn(const DataFunction &fn)
  {
    m_data_fn = fn;
  }

 private:
  /** Remote IP address of the data service. */
  const std::string m_address;

  /** Remote port number of the data service. */
  const std::size_t m_port;

  /** Service intialization string. */
  const std::string m_initialize;

  /** Callback function for incoming data. */
  DataFunction m_data_fn;
  State<Mutex,Lock> m_state;
  blocking_bool<Mutex,Lock,Condition> m_running;

  template <
    typename SamplerT,
    typename ThreadT,
    typename MutexT,
    typename LockT,
    typename ConditionT
  >
  friend class Manager;
}; // class Reader

/**
  Template based routing of input stream to the appropriate Format method.
  Required since Format does not expose a public template map generation
  interface.
*/
template <typename FormatType>
FormatType format_data(const Client::data_type &data);

template <>
Format::configurable_service_type format_data(const Client::data_type &data);

template <>
Format::preview_service_type format_data(const Client::data_type &data);

template <>
Format::sensor_service_type format_data(const Client::data_type &data);

template <>
Format::raw_service_type format_data(const Client::data_type &data);


/**
  Container for Reader and Sampler objects. Run a Reader thread
  for each Host:Port pair, route the data to any Sampler objects
  that are currently attached. A Reader thread lasts only as long
  as there is at least one Sampler attached.
*/
template <
  typename SamplerType,
  typename Thread=boost::thread,
  typename Mutex=boost::recursive_mutex,
  typename Lock=boost::recursive_mutex::scoped_lock,
  typename Condition=boost::condition_variable_any
>
class Manager {
 public:
  typedef SamplerType sampler_type;
  typedef typename sampler_type::data_type data_type;

  Manager()
    : m_id(), m_container(), m_mutex()
  {
  }

  ~Manager()
  {
    lock_type lock(m_mutex);

    BOOST_FOREACH (typename container_type::value_type &item, m_container) {
      item.second.close();
    }
  }

  bool attach(sampler_type &sampler)
  {
    if (0 != sampler.m_sampler_id) {
      // Already attached, or some other error.
#if MOTION_SDK_USE_EXCEPTIONS
      throw detail::error("sampler already attached to data stream");
#endif  // MOTION_SDK_USE_EXCEPTIONS
      return false;
    }

    if (0 == sampler.m_port) {
      // Invalid port number.
#if MOTION_SDK_USE_EXCEPTIONS
      throw detail::error("sampler specifies invalid port number of 0");
#endif  // MOTION_SDK_USE_EXCEPTIONS
      return false;
    }

    // Create the map key that we can reference this address:port
    // pair with.
    typedef typename container_type::value_type::first_type key_type;
    const key_type key(sampler.m_address, sampler.m_port, sampler.m_initialize);

    // Obtain the lock on the node container state.
    lock_type lock(m_mutex);

    // Look up the thread attached to the address:port pair
    // requested by the incoming sampler.
    typename container_type::iterator itr = m_container.find(key);
    if (itr == m_container.end()) {
      itr = m_container.insert(
        std::make_pair(key, Node())).first;
      if (m_container.end() == itr) {
        // Insertion failure.
#if MOTION_SDK_USE_EXCEPTIONS
        throw detail::error("failed to insert sampler record");
#endif  // MOTION_SDK_USE_EXCEPTIONS
        return false;
      } else {
        // Initialize the Node.
        // 1. Spawn a communications thread.
        boost::shared_ptr<typename Node::reader_type> reader(
          new typename Node::reader_type(sampler.m_address,
                                         sampler.m_port,
                                         sampler.m_initialize));

        itr->second.m_reader = reader;
        itr->second.m_thread = boost::shared_ptr<Thread>(new Thread(
          boost::bind(&Node::reader_type::operator(), boost::ref(*reader))));

        // Here, we may want to wait until the thread is actually running, and the
        // connection attempt has been made.
        reader->m_running.time_out(5);
        if (reader->m_running << true) {
          // Wait to register the data callback function until we
          // have a successful running thread.
          //reader->m_data_fn = boost::bind(&Manager::set_data_slot, this, 1, _1);
        } else {
          // Clean up the thread. It did not start up successfully. Probably
          // a failed connection to the data host.
          reader->quit(true);
          if (NULL != itr->second.m_thread) {
            itr->second.m_thread->join();
          }

          m_container.erase(itr);
          itr = m_container.end();

#if MOTION_SDK_USE_EXCEPTIONS
          throw detail::error(
            "failed to start data stream communication thread");
#endif  // MOTION_SDK_USE_EXCEPTIONS
          return false;
        }
      }
    } else if (itr->second.m_reader->m_state.quit()) {
      // This thread is no longer running, but a sampler is still
      // attached. We will never be able to read any data from this
      // sampler, so fail now.
#if MOTION_SDK_USE_EXCEPTIONS
      throw detail::error(
        "failed to attach to existing, but closed, data stream");
#endif  // MOTION_SDK_USE_EXCEPTIONS
      return false;
    }

    // At this point, itr must be a valid iterator. The above code
    // enforces this, but be verbose.
    if (itr == m_container.end()) {
#if MOTION_SDK_USE_EXCEPTIONS
      throw detail::error("failed to retrieve sampler record");
#endif  // MOTION_SDK_USE_EXCEPTIONS
      return false;
    }

    // 1. Assign the newly attached sampler a unique id.
    sampler.m_sampler_id = ++m_id;
    // 2. Store the sampler in our local container.
    itr->second.m_sampler_container.push_back(sampler);
    // 3. Associate the sampler and thr reader thread state objects.
    sampler.m_state = itr->second.m_reader->m_state;
    // 4. Register the data callback functon.
    itr->second.m_reader->m_data_fn =
      boost::bind(&Manager::set_data_slot, this, key, _1);

    return true;
  }

  bool detach(sampler_type &sampler)
  {
    if (0 == sampler.m_sampler_id) {
      // Not currently attached, or some other error.
#if MOTION_SDK_USE_EXCEPTIONS
      throw detail::error("sampler not attached to data stream");
#endif  // MOTION_SDK_USE_EXCEPTIONS
      return false;
    }

    lock_type lock(m_mutex);

    // Create the map key that we can reference this address:port pair with.
    typedef typename container_type::value_type::first_type key_type;
    const key_type key(sampler.m_address, sampler.m_port, sampler.m_initialize);

    // Look up the thread attached to the address:port pair
    // requested by the incoming sampler.
    typename container_type::iterator node_itr = m_container.find(key);
    if (m_container.end() != node_itr) {
      // Iterate through all Sampler objects listening for data on from this
      // address:port:XML tuple.
      typename Node::sampler_container_type::iterator itr =
        node_itr->second.m_sampler_container.begin();
      for (; itr!=node_itr->second.m_sampler_container.end(); ++itr) {
        if (sampler.m_sampler_id == itr->m_sampler_id) {
          break;
        }
      }

      if (node_itr->second.m_sampler_container.end() != itr) {
        node_itr->second.m_sampler_container.erase(itr);
        sampler.m_sampler_id = 0;

        // If we just remove the last sampler that is attached
        // to this reader thread, close down the thread.
        if (node_itr->second.m_sampler_container.empty()) {
          node_itr->second.m_reader->m_data_fn = typename Node::function_type();
          node_itr->second.m_reader->quit(true);
          if (NULL != node_itr->second.m_thread) {
            node_itr->second.m_thread->join();
          }

          m_container.erase(node_itr);
        }
      }
    }

    return true;
  }

private:
  typedef Thread thread_type;
  typedef Mutex mutex_type;
  typedef Lock lock_type;
  typedef Condition condition_type;

  class Node {
   public:
    typedef boost::function<
      bool (const Client::data_type &)
    > function_type;

    typedef Reader<
      function_type,
      mutex_type,
      lock_type,
      condition_type
    > reader_type;

    typedef std::vector<
      sampler_type
    > sampler_container_type;

    boost::shared_ptr<Thread> m_thread;
    boost::shared_ptr<reader_type> m_reader;
    sampler_container_type m_sampler_container;

    void close()
    {
      if (m_sampler_container.empty()) {
        m_reader->set_data_fn(function_type());
        m_reader->quit(true);
        if (m_thread && m_thread->joinable()) {
          m_thread->join();
        }

        m_sampler_container.clear();
      }
    }
  }; // class Node

  class NodeKey {
   public:
    NodeKey(const std::string &hostname, const std::size_t &port,
            const std::string &initialize)
      : m_data(std::make_pair(hostname, port), initialize)
    {
    }

    bool operator<(const NodeKey &rhs) const
    {
      return (m_data < rhs.m_data);
    }

    bool operator==(const NodeKey &rhs) const
    {
      return (m_data == rhs.m_data);
    }

    bool operator>(const NodeKey &rhs) const
    {
      return (m_data > rhs.m_data);
    }

   private:
    /**
      Store the data in this crazy way to make the comparison operators a piece
      of cake. Public clients will not access the data members anyways.
    */
    const std::pair<
      std::pair<std::string,std::size_t>,
      std::string
    > m_data;
  }; // class NodeKey

  typedef std::map<
    NodeKey,
    Node
  > container_type;

  std::size_t m_id;
  container_type m_container;
  mutex_type m_mutex;

  bool set_data_slot(const typename container_type::key_type &key,
                     const Client::data_type &data)
  {
    bool result = false;

    data_type in_data;
    if (!data.empty()) {
      // Generate the formatted version of the incoming data.
      in_data = format_data<data_type>(data);
    }

    // Obtain exclusive lock on the node container state.
    lock_type lock(m_mutex);

    typename container_type::iterator node_itr = m_container.find(key);
    if (m_container.end() != node_itr) {
      // Iterate through all Sampler objects listening for data on
      // from this address:port pair.
      if (!data.empty() || node_itr->second.m_reader->m_state.quit()) {
        typename Node::sampler_container_type::iterator itr=node_itr->second.m_sampler_container.begin();
        for (; itr!=node_itr->second.m_sampler_container.end();) {
          if (itr->set_data(in_data)) {
            ++itr;
            result = true;
          } else {
            itr = node_itr->second.m_sampler_container.erase(itr);
          }
        }
      }
    }

    return result;
  }

}; // class Manager


// Define this to include the actual implementations in your client program. Do
// it this way to avoid linking problems.
#if defined(MOTION_SDK_PLUGIN_DEVICE_IMPL)

template <>
Format::configurable_service_type format_data(const Client::data_type &data)
{
  return Format::Configurable(data.begin(), data.end());
}

template <>
Format::preview_service_type format_data(const Client::data_type &data)
{
  return Format::Preview(data.begin(), data.end());
}

template <>
Format::sensor_service_type format_data(const Client::data_type &data)
{
  return Format::Sensor(data.begin(), data.end());
}

template <>
Format::raw_service_type format_data(const Client::data_type &data)
{
  return Format::Raw(data.begin(), data.end());
}

#endif  // MOTION_SDK_PLUGIN_DEVICE_IMPL

}}}  // namespace Motion::SDK::Device

#endif  // __MOTION_SDK_PLUGIN_DEVICE_HPP_
