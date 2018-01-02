#!/usr/bin/python

import time

class State:
    def __init__(self, max_hit_count = None, interval = None):
        self.begin         = 0.0
        self.hit_count     = 0.0
        self.max_hit_count = float(max_hit_count)
        self.interval      = float(interval)
        self.banned        = False

    def ban(self):
        self.banned = True


def handle_request(state, cur_time):
    # If begin is zero, that means this is the first time we're
    # hearing about this IP address.
    if abs(state.begin) < 0.000001:
        state.begin = cur_time

    #
    #     time_window_movement
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

    if time_window_movement > 0:
        # Situation:
        # I.e. the interval we've measured is bigger than state.interval.
        # ... -------x-----<----------------------------->

        # Add one hit
        state.hit_count += 1

        # "Cut out" a portion of hits_count corresponding to time_window_movement
        state.hit_count -= state.hit_count * time_window_movement / (cur_time - state.begin)

        # In the next line we "move" begin so that
        # (cur_time - begin) is equal to state.interval.
        # Or graphically, our situation will become like this:
        #                  x
        # ... -------------<----------------------------->
        state.begin = state.begin + time_window_movement

        # Floating point error correction to ensure hit_count is not negative.
        state.hit_count = max(0, state.hit_count)
    else:
        # Situation:
        # ......... -------<-----x----------------------->
        # That is, we haven't yet measured for more than 
        # state.interval time. So just increment rate by adding one hit.
        state.hit_count += 1

    if state.hit_count >= state.max_hit_count:
        state.ban()


def now():
    return time.time()

def test_1():
    s = State(max_hit_count = 5, interval = 5)

    HITS = 5

    for hit in range(1, HITS + 1):
        handle_request(s, float(hit) / 2)

        if hit < 5:
            assert s.banned == False
        else:
            assert s.banned == True


def test_2():
    s = State(max_hit_count = 30, interval = 60)

    HITS = 30

    for hit in range(1, HITS + 1):
        handle_request(s, float(hit))

        if hit < 30:
            assert s.banned == False
        else:
            assert s.banned == True


if __name__ == "__main__":
    test_1()
    test_2()

    ## Uncomment if you wish to test this manually.
    #s = State(max_hit_count = 5.0, interval = 5.0)
    #while s.banned == False:
    #    raw_input("Hit enter to simulate a request")
    #    handle_request(s, now())
