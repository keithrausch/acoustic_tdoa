import numpy as np
from enum import Enum
import copy

class HypothesisStatus(Enum):
    IMMATURE = 0
    MATURE = 1
    GRADUATE = 2
    FAILED = 3

class Hypothesis:
    def __init__(self, id, args, time_s, det_val):
        self.id = id
        self.meas_history = ''
        self._args = args
        # self._nDets = 0
        self._mn_buffer = [True for _ in range(args['n'])]
        self._step_count = 0
        self._last_det_val = det_val
        self._last_time_s = time_s
        self._next_time_s = time_s + self._args['sync_period_s']

    def _update_latest(self, det_time_s, det_val):

        self._last_time_s = det_time_s
        self._next_time_s = det_time_s + self._args['sync_period_s']
        self._last_det_val = det_val

        # self._nDets += 1

    def _update_step_counts(self, is_hit):
        self._mn_buffer[self._step_count % self._args['n']] = is_hit
        self._step_count += 1

    def gate(self):
        uncertainty_radius = self._args['uncertainty_radius_s']
        lower_gate_s = self._next_time_s - uncertainty_radius
        upper_gate_s = self._next_time_s + uncertainty_radius
        return lower_gate_s, upper_gate_s

    
    def is_in_gate(self, time_s):
        gate_s = self.gate()
        return gate_s[0] <= time_s and time_s <= gate_s[1]

    def split(self, det_time_s, det_val):
        if self.is_in_gate(det_time_s):
            ret = copy.deepcopy(self)
            ret._update_latest(det_time_s, det_val)
            ret._update_step_counts(True)
            return ret
        else:
            return None
        
    def miss(self):
        self._next_time_s += self._args['sync_period_s']
        self._update_step_counts(False)

    def is_good(self):
        return np.sum(self._mn_buffer) > self._args['m']
    
    # def gate_overlaps_window(self, window_start_s, window_end_s):
    #     return window_start_s <= self._gate_s[0] and self._gate_s[1] <= window_end_s

    def to_string(self):
        mn_str = ','.join([('T' if x else 'F') for x in self._mn_buffer])
        return f"id = {self.id}, meas_history={self.meas_history}, last_time = {self._last_time_s}, next_time = {self._next_time_s}, {mn_str}"

def print_hypotheses(hypotheses):
    for hyp in hypotheses:
        print(hyp.to_string())

def filter_one(filter_args, hyp_args, hypotheses_in, window_start_s, window_end_s, detections):

    hypotheses_out = []
    for reltime_s, det_val in detections:
        det_time_s = window_start_s + reltime_s
        
        # apply this det to all existing hypotheses
        det_was_used = False
        for hyp in hypotheses_in:
            updated_hyp = hyp.split(det_time_s, det_val)
            if updated_hyp is not None:
                updated_hyp.id = filter_args['global_id']
                filter_args['global_id'] += 1
                updated_hyp.meas_history += f',{filter_args['det_id']}'
                hypotheses_out.append(updated_hyp)
                det_was_used = True

        # if no hypothesis used it, then create a new hypothesis
        if not det_was_used:
            id = filter_args['global_id']
            filter_args['global_id'] += 1
            hyp = Hypothesis(id, hyp_args, det_time_s, det_val)
            hyp.meas_history += f',{filter_args['det_id']}'
            print(f'creating track {id}')
            hypotheses_out.append(hyp)
            
        filter_args['det_id'] += 1
    
    for hyp in hypotheses_in:
        # propagate this hypothesis forward, but only if needed
        hyp_gate_is_stale = hyp.gate()[1] < window_start_s
        if hyp_gate_is_stale:
            # propagate this hypothesis as a miss
            hyp.miss()
            if hyp.is_good():
                hypotheses_out.append(hyp)
            else:
                print(f'killing track {hyp.id}')
        else:
            hypotheses_out.append(hyp) # gate endpoint is in the future, keep this track around

    return hypotheses_out


def main():

    np.random.seed(4072785483)

    N_samples_per_block = 128
    sync_period_s = 1.0
    block_period_s = 0.25

    # cd_freq = 44100.0
    cd_freq = 32.0
    uncertainty_radius_samples = 2
    uncertainty_radius_s = uncertainty_radius_samples * 1.0 / cd_freq
    hyp_args = {'sync_period_s':sync_period_s, 'uncertainty_radius_s':uncertainty_radius_s, 'm':5, 'n':5}
    filter_args = {'global_id':0, 'det_id':0}

    hypotheses = []

    dets_list = []
    for i in range(80):
        N_dets_per_block = 5
        window_start_s = i*block_period_s
        det_reltimes_in_this_block = np.random.rand(N_dets_per_block)*N_samples_per_block / cd_freq
        det_mags_in_this_block = np.random.rand(5)
        # det_offsets_in_this_block[0] = 0 # set first one to the true signal
        # det_mags_in_this_block[0] = 1.0
        if i%4==0:
            det_reltimes_in_this_block[0] = 0
            print(f'true signal det id: {i*N_dets_per_block}')

        dets = list(zip(det_reltimes_in_this_block, det_mags_in_this_block))

        dets_list.append((window_start_s, dets))


    for window_start_s, detections in dets_list:
        window_end_s = window_start_s + block_period_s
        hypotheses = filter_one(filter_args, hyp_args, hypotheses, window_start_s, window_end_s, detections)
        hypotheses.sort(key=lambda hyp: (len(hyp.meas_history), hyp.id), reverse=False)
        # hypotheses.sort(key=lambda hyp: hyp.id, reverse=False)
        print_hypotheses(hypotheses)
        print(f'totaling {len(hypotheses)} hypotheses')
        print('\n')




if __name__ == "__main__":
    main()