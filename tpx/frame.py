from . backend import numpy

from pathlib import Path

from c_tpx import parse_binary_file

class frame(object):

    hot_pixels = numpy.asarray([51533,])

    def __init__(self, * , fname : str = None, data : numpy.ndarray = None):

        if fname is not None:
            path = Path(fname)
            if path.exists():
                self.data = parse_binary_file(fname)
            else:
                raise FileNotFoundError(fname)
        elif data is not None:
            self.data = data

        self.shape = self.data.shape
        self.dtype = self.data.dtype

        if fname is not None:
            # Only apply normalization straight from file:
            self.data['TOA'] -= numpy.min(self.data['TOA'])
            self.data['TOA'] *= 1e-9
            self.data['x'] -= 260

        # Map the x/y into a global index:
        self.global_index = numpy.ravel_multi_index(
            [self.data['x'], self.data['y']],
            [256,256]
        )

        # # find all pixels to mask:
        # mask = self.global_index != 51533
        # self.data = self.data[mask]

    def __getitem__(self, key):
        return self.data[key]
    
    def __len__(self):
        return len(self.data)

    def threshold(self, ToT_threshold = 0.0):

        where = self.data['TOT'] > ToT_threshold
        return frame(data=self.data[where])


    def threshold_min(self, ToT_threshold = 1e6):

        where = self.data['TOT'] < ToT_threshold
        return frame(data=self.data[where])

    def time_slice(self, start_time, end_time):
        where = self.data['TOA'] >= start_time
        d = self.data[where]
        where = d['TOA'] <= end_time
        return frame(data=d[where])

# Define a hit dtype:
hit_dtype = numpy.dtype([
    ('x',   numpy.float32),
    ('y',   numpy.float32),
    ('TOA', numpy.float32),
    ('TOT', numpy.float32),
    ('n',   numpy.uint16 ),
    ]
)

def analyze_frame(input_arr : numpy.ndarray, time_gap_threshold : float):

    # Pull off the times:
    times = input_arr["TOA"]

    # Get the gap between each pixel hit:
    gaps = times[1:] - times[:-1]
    slices_edges = numpy.where(gaps > time_gap_threshold)[0]


    # hit_tot = []
    # hit_x   = []
    # hit_y   = []
    # hit_t   = []
    # hit_n   = []

    hits = []
    start = 0
    # Loop over and pull the hits into every gap:
    for i_end in slices_edges:
        # Slice the raw pixels for this hit:
        this_hit = input_arr[start:i_end+1]

        total_charge = numpy.sum(this_hit["TOT"])
        hit_tot = total_charge

        # The x and y locations are weighted by the charge on each pixel
        hit_x = numpy.sum(this_hit["TOT"]*this_hit["x"]) / total_charge
        hit_y = numpy.sum(this_hit["TOT"]*this_hit["y"]) / total_charge
        
        # The time is defined as the first time of this cluster:
        hit_t = numpy.min(this_hit["TOA"])
        hit_n = len(this_hit)
        
        start = i_end+1

        hits.append(numpy.array((hit_x, hit_y, hit_t, hit_tot, hit_n), dtype=hit_dtype))

    return numpy.stack(hits, axis=-1)

class hits(object):
    """
    from a frame, group the signals as hits
    """

    def __init__(self, * , 
                 input_frame: frame = None, 
                 input_hits : numpy.ndarray = None,
                 minimum_time_separation = 1e-5):

        if input_frame is not None:
            self.data = analyze_frame(input_frame.data, minimum_time_separation)
        elif input_hits is not None:
            self.data = input_hits

        # For every recorded pixel, we group them into gaps based on time.
        # Each output contains the following info:
        self.shape = self.data.shape
        self.dtype = self.data.dtype

    def __getitem__(self, key):
        return self.data[key]

    def __len__(self):
        return len(self.data)

    def threshold(self, ToT_threshold = 0.0):

        where = self.data['TOT'] > ToT_threshold
        return hits(input_hits=self.data[where])

    def threshold_n(self, n_threshold = 1):

        where = self.data['n'] > n_threshold
        return hits(input_hits=self.data[where])
