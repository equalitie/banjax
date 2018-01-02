#!/usr/bin/python

import time

class State:
    def __init__(self, max_hit_count = None, interval = None):
        self.begin         = None
        self.hit_count     = 0.0
        self.max_hit_count = float(max_hit_count)
        self.interval      = float(interval)
        self.banned        = False

    def ban(self):
        self.banned = True


def handle_request(state, cur_time):
    # If begin is zero, that means this is the first time we're
    # hearing about this IP address.
    if state.begin == None:
        state.begin = cur_time

    #
    #        time_window
    #         <----->
    #         |     |
    #         |     |                       cur_time
    # ... ----x-----<-------------------------->
    #       begin   |                          |
    #         |     |                          |
    #         |     |      state.interval      |
    #         |     <-------------------------->
    #         |                                |
    #         <-------------------------------->
    #                   cur_interval
    #
    cur_interval = cur_time - state.begin

    if cur_interval > state.interval:
        time_window = cur_interval - state.interval
        # Add one hit
        state.hit_count += 1
        # "Cut out" a portion of `hit_count` corresponding to time_window.
        state.hit_count -= state.hit_count * time_window / cur_interval
        # We "move" `begin` so that cur_interval is equal to state.interval.
        state.begin = cur_time - state.interval
        # Floating point error correction to ensure hit_count is not negative.
        state.hit_count = max(0, state.hit_count)
    else:
        # We have not yet exceeded the `state.interval`, so just increase
        # `hit_count` by one.
        state.hit_count += 1

    if state.hit_count > state.max_hit_count:
        state.ban()


def test_1():
    s = State(max_hit_count = 5, interval = 5)

    HITS = 6

    for hit in range(1, HITS + 1):
        # We divide `hit` by two to not exceed the interval.
        handle_request(s, hit / 2.0)

        if hit <= 5: assert s.banned == False
        else:        assert s.banned == True


def test_2():
    s = State(max_hit_count = 30, interval = 60)

    HITS = 31

    for hit in range(1, HITS + 1):
        handle_request(s, float(hit))

        if hit <= 30: assert s.banned == False
        else:         assert s.banned == True


if __name__ == "__main__":
    test_1()
    test_2()

    ## Uncomment if you wish to test this manually.
    #s = State(max_hit_count = 5.0, interval = 5.0)
    #while s.banned == False:
    #    raw_input("Hit enter to simulate a request")
    #    handle_request(s, time.time())
