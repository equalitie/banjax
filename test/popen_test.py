import pexpect
import asyncio

import random
import string

from banjax_behavior_test import do_curl


if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    loop.run_until_complete(main())
