#!/usr/bin/python

import time

class State:
    begin             = None
    rate              = None
    hits_per_interval = None
    interval          = None
    max_rate          = None
    banned            = False

    def __init__(self, hits_per_interval = None, interval = None):
        self.begin = 0.0
        self.rate = 0.0
        self.hits_per_interval = float(hits_per_interval)
        self.interval = float(interval)
        self.max_rate = self.hits_per_interval / self.interval


    def ban(self):
        #print "Banned!\n"
        self.banned = True


def handle_request(state, cur_time):
    # If begin is zero, that means this is the first time we're
    # hearing about this IP address.
    if abs(state.begin) < 0.000001:
        state.begin = cur_time

    #
    #         time_window_movement
    #         <----->
    #         |     |
    #         |     |                       cur_time
    # ... ----x-----<-------------------------->
    #       begin   |                          |
    #               |                          |
    #               |      state.interval      |
    #               <-------------------------->
    #
    time_window_movement = cur_time - state.begin - state.interval

    # Same as `if (cur_time - begin) > state.interval`.
    # That is, "if the time of our measurement exceeded the state.interval"
    if time_window_movement > 0:
        # Situation:
        # ... -------x-----<----------------------------->
        # I.e. the interval we've measured is bigger than state.interval.
        # So in the next line we "move" begin so that
        # (cur_time - begin) is equal to state.interval.
        # Or graphically, our situation will become like this:
        #                  x
        # ... -------------<----------------------------->
        state.begin = state.begin + time_window_movement
        # Now, since we "cut out" the time_window_movement part of our
        # working interval, we need to adjust our rate accordingly.
        # I'll break the original formula:
        # rate = rate - (rate * time_window_movement - 1) / state.interval
        # into two:
        # 1. "Cut out" a portion of rate corresponding to time_window_movement
        state.rate = state.rate - state.rate * time_window_movement / state.interval
        # and:
        # 2. Add one hit
        state.rate += 1/state.interval
        # I think this is just a floating point error correction to ensure
        # rate is not less than zero.
        if state.rate < 0:
            state.rate = 1/state.interval
    else:
        # Situation:
        # ......... -------<-----x----------------------->
        # That is, we haven't yet measured for more than 
        # state.interval time. So just increment rate by adding one hit.
        state.rate += 1/state.interval

    #print("rate:" + repr(state.rate) + " max_rate:" + repr(state.max_rate))

    if state.rate >= state.max_rate:
        state.ban()


def now():
    return time.time()

def test_1():
    s = State(hits_per_interval = 5, interval = 5)

    HITS = 5

    for hit in range(0, HITS):
        handle_request(s, float(hit) / 2)

        if hit < 4: # 0,1,2,3
            assert s.banned == False
        else: # 4
            assert s.banned == True


def test_2():
    s = State(hits_per_interval = 30, interval = 60)

    HITS = 31

    for hit in range(0, HITS):
        handle_request(s, float(hit))

        if hit < 30:
            assert s.banned == False
        else:
            assert s.banned == True



if __name__ == "__main__":
    test_1()
    test_2()

    # Uncomment if you wish to test this manually.
    #s = State(hits_per_interval = 5.0, interval = 5.0)
    #while s.banned == False:
    #    raw_input("Hit enter to simulate a request")
    #    handle_request(s, now())
