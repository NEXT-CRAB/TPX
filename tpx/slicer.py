from . backend import numpy
from . frame import frame


def slice(tpx_frame, bin_size, threshold=2, method="toa_counts"):

    """
    Take the whole frame, bin the hits into time-separated regions
    """
    # First, bin the data:
    time_bins, non_zero_bins, non_zero_times, non_zero_counts = \
        bin_tpx_data_for_slicing(tpx_frame, bin_size, threshold, method)

    lower_bounds, upper_bounds = select_regions_of_interest(non_zero_bins, time_bins)

    events = slice_frame_into_events(tpx_frame, lower_bounds, upper_bounds)

    return events


def bin_tpx_data_for_slicing(tpx_frame, bin_size=100e-6, threshold=2, method="toa_counts"):
    """Bin the data to accomodate event slicing.  Parameters:

    Args:
        tpx_frame (tpx.Frame): tpx.Frame object to be sliced
        bin_size (_type_): total bin size in seconds (ie, 1 us would be 1e-6)
        threshold (_type_): Threshold to apply to each bin
        method (str, optional): method of slicing, either "toa_counts" or "tot_sum". Defaults to "toa_counts".
    """
    
    # First, create a list of bins:

    max_time = numpy.max(tpx_frame["TOA"])

    hit_times = tpx_frame["TOA"]

    time_bins = numpy.arange(0,max_time, bin_size)

    # Bin all the hit times into the bins to determine how much activity was in each bin:
    binned_activity, bin_edges = numpy.histogram(hit_times, bins=time_bins)
    bin_centers = 0.5*(bin_edges[1:] + bin_edges[:-1])


    # Apply the bin thresholds:

    if method == "toa_counts":
        non_zero_bins   = binned_activity >= threshold
        non_zero_times  = bin_centers[non_zero_bins]
        non_zero_counts = binned_activity[non_zero_bins]
    elif method == "tot_sum":
        # To do this based on total ToT per bin is trickier and slower but not really slow:
        tot_per_bin = numpy.asarray([
            numpy.sum(hit_tot[ numpy.where((hit_times > low) & (hit_times <= high)) ])
            for low, high in zip(time_bins[:-1], time_bins[1:])
        ])
        non_zero_bins   = tot_per_bin >= tot_threshold
        non_zero_times  = bin_centers[non_zero_bins]
        non_zero_counts = tot_per_bin[non_zero_bins]

    return time_bins, numpy.where(non_zero_bins)[0], non_zero_times, non_zero_counts


def select_regions_of_interest(non_zero_bins, original_bins):
    """Take the list of bins and return the regions of interest in min/max.

    Args:
        non_zero_bins (numpy.ndarray[int]): List of histogram bins from a frame that are not zero
        original_bins (numpy.ndarray): The original histogram's bins

    Returns:
        _type_: _description_
    """


    bin_spacing = non_zero_bins[1:] - non_zero_bins[:-1]
    # this variable says whether the bin at [i+1] is directly adjacent to [i]

    temp_adjacent_bins = bin_spacing == 1
    # in order to select original bin indexes from this, pad it with a False:

    adjacent_bins = numpy.zeros_like(non_zero_bins, dtype="bool")
    # And then add the originals in:
    adjacent_bins[1:] = temp_adjacent_bins

    # The easiest way to vectorize this is the following:
    # For the list of non-adjacent bins, we just select the boundaries from the original histogram bins.
    # For the list of adjacent bins, we extend the bins in the selected bins

    # We never start a region of interest on a bin that has an adjacency
    starting_indexes = non_zero_bins[adjacent_bins != 1]
    ending_indexes   = starting_indexes + 1

    extend_bins = non_zero_bins[adjacent_bins == 1]

    # For each bin in the "extend bins" category, we find the matching end bin and increment it:
    for bin in extend_bins:
        index = numpy.where(ending_indexes == bin)[0]
        ending_indexes[index] += 1

    # TODO: The above loop does not work if there are multiple consecutive bins to merge!


    # Finally, take the bin indexes and use them in the original bins to get upper and lower bounds:
    lower_bounds = original_bins[starting_indexes]
    upper_bounds = original_bins[ending_indexes]

    return lower_bounds, upper_bounds


def slice_frame_into_events(tpx_frame, lower_bounds, upper_bounds):
    """Time-slice the 

    Args:
        tpx_frame (tpx.frame): original frame of events
        lower_bounds (numpy.ndarray): Array of times, in [seconds], for the upper bounds
        upper_bounds (numpy.ndarray): Array of times, in [seconds], for the lower bounds

    Returns:
        List[tpx.frame]: List of frames sliced by the suggested bounds
    """

    events = [
        tpx_frame.time_slice(lower, upper) for lower, upper in zip(lower_bounds, upper_bounds)
    ]

    return events