/*
  @file    tools/sdk/js/motion.sdk.js
  @author  Luke Tokheim, luke@motionnode.com
  @version 2.2

  Copyright (c) 2015, Motion Workshop
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
(function(window) {"use strict";

/** Initialize the module by name. */
window.Motion || (window.Motion = {VERSION: 0});

/**
  Implements a simple socket based client to stream data from the Motion
  Service. Provide a simple interface read real-time Motion data from from a
  client side JavaScript application. Updates at half the actual measurement
  rate to save on processing time.

  Requires browser support for the HTML5 WebSocket and Typed Array standards.
    http://caniuse.com/websocket
    http://caniuse.com/typedarrays

  @code
  // Open the socket connection and start streaming data.
  var client = new Motion.Client();
  
  // Provide callbacks for data and configuration frames.
  client.readData(function(data) {
    // We received a frame of data. This is 8 floating point numbers per
    // channel. Channel names must be retrieved from the JSON string.
    //update(data);
  }, function(json) {
    // We received a list of channels in JSON string format. This may happen
    // mid stream if the channel list changes.
    //update_json(json);
  });
  @endcode
*/
Motion.Client = function (options) {
  var self = this;

  if (undefined === options) {
    options = {};
  }
  var host =
    (undefined !== options.host) ? options.host : window.location.hostname;
  var port = (undefined !== options.port) ? options.port : 32074;
  var path = (undefined !== options.path) ? options.path : "/stream/binary";
  var url = [
    "ws://", host.toString(), ":", port.toString(), path.toString()
  ].join("");

  this.callback = undefined;
  this.json_callback = undefined;
  this.socket = new WebSocket(url);

  // WebSocket callback. The connection to the server is now open.
  this.socket.onopen = function () {
    self.socket.binaryType = "arraybuffer";
  };

  // WebSocket callback. The previous connection to the server is now closed.
  this.socket.onclose = function () {
    if (self.callback) {
      self.callback();
    }    
  };

  // WebSocket callback. There was an error in the data stream.
  this.socket.onerror = function () {
    if (self.socket) {
      self.socket.close();
    }
  };

  // WebSocket callback. We received a message on the current data stream.
  this.socket.onmessage = function(event) {
    if (event.data instanceof ArrayBuffer) {
      // The incoming message is in binary format. This is the most recent
      // sample from the server.
      if (self.callback) {
        self.callback(event.data);
      }
    } else {
      // String formatted message. This defines the channel names and ordering
      // for all subsequent samples.
      if (self.json_callback) {
        self.json_callback(event.data);
      }
    }
  };
};

Motion.Client.prototype = {
  constructor: Motion.Client,

  readData: function(callback, json_callback) {
    this.callback = callback;
    this.json_callback = json_callback;
  },

  close: function() {
    this.socket.close();
  }
};

})(window);