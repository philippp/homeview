#!/usr/bin/env python
import os
from server import application

if __name__ == "__main__":
  os.chdir(os.path.dirname(__file__))
  application.run()
