#
# @file    tools/sdk/python/build/setup.py
# @author  Luke Tokheim, luke@motionnode.com
# @version 2.0
#
#
# (C) Copyright Motion Workshop 2013. All rights reserved.
#
# The coded instructions, statements, computer programs, and/or related
# material (collectively the "Data") in these files contain unpublished
# information proprietary to Motion Workshop, which is protected by
# US federal copyright law and by international treaties.
#
# The Data may not be disclosed or distributed to third parties, in whole
# or in part, without the prior written consent of Motion Workshop.
#
# The Data is provided "as is" without express or implied warranty, and
# with no claim as to its suitability for any purpose.
#
import ez_setup
ez_setup.use_setuptools()

from setuptools import setup, find_packages

setup(name = 'MotionSDK',
      version = '2.0',
      packages = find_packages(),
      py_modules = ['MotionSDK'],
      description = 'Motion Software Development Kit (SDK)',
      author = 'Motion Workshop',
      author_email = 'info@motionnode.com',
      url = 'http://www.motionnode.com/tools/sdk/python/',
      license='Proprietary'
      )
