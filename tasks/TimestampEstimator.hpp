#ifndef AGGREGATOR_TIMESTAMP_ESTIMATOR_HPP
#define AGGREGATOR_TIMESTAMP_ESTIMATOR_HPP

#include <base/Time.hpp>
#include <base/CircularBuffer.hpp>
#include <vector>

#include <aggregator/TimestampEstimatorStatus.hpp>

namespace aggregator
{
    /** The timestamp estimator takes a stream of samples and determines a best
     * guess for each of the samples timestamp.
     *
     * It assumes that most samples will be received at the right period. It
     * will not work if the reception period is completely random.
     */
    class TimestampEstimator
    {
        /** To avoid loss of precision while manipulating doubles, we move all
         * times to be relative to this time
         *
         * It gets added back when returning from update() and updateReference()
         */
        base::Time m_zero;

        /** The requested estimation window */
        double m_window;

        /** Set of uncorrected timestamps that is at most m_window large.
         * Negative values are placeholders for missing samples
	 */
        boost::circular_buffer<double> m_samples;

        /** The last estimated timestamp, without latency
         *
         * The current best estimate for the next sample, with no new
         * information, is always m_last + getPeriod() - m_latency
         */
        double m_last;

        /** During the estimation, this vector keeps track of consecutive
         * samples where the difference between the estimated and provided
         * timestamps is greater than a period
         *
         * When more than m_lost_threshold of such samples are received, we
         * assume that we lost samples
         */
        std::vector<long> m_lost;

        /** if m_lost.size() is greater than m_lost_threshold, we consider
	 * that we lost some samples
         */
        int m_lost_threshold;

        /** The total estimated count of lost samples so far */
        int m_lost_total;

        /** Set to true the first time the window got full. It is used when an
         * initial period is provided, to determine whether getPeriodInternal
         * should use the initial period or only raw data
         */
        bool m_got_full_window;

        double getPeriodInternal() const;

        /** During the estimation, we keep track of when we encounter an actual
         * sample that matches the current estimated base time.
         *
         * If we don't encounter one in a whole estimation window, we assume
         * that something is wrong and that we should reset it completely
         */
        double m_base_time_reset;

        /** The offset between the last base time and the new base time at the
         * last call to resetBaseTime 
         *
         * Used for statistics / monitoring purposes only
         */
        double m_base_time_reset_offset;

        /** The latency, i.e. the fixed (or, more accurately, slow drifting)
         * difference between the incoming timestamps and the actual timestamps
         *
         * An apriori value for it can be provided to the constructor, and it
         * will be updated if updateReference is used (i.e. external timestamps
         * are available).
         *
         * It is used as:
         *
         * <code>
         * actual_timestamp = incoming_timestamp - latency
         * </code>
         */
	double m_latency;

        /** The raw latency, i.e. the unprocessed difference between the
         * estimator's last estimated time and last received reference time
         */
        double m_latency_raw;

        /** Apriori latency provided to the estimator's constructor
         *
         * Zero if none is provided
         *
         * See m_latency for explanations
         */
	double m_initial_latency;

        /** Maximum value taken by the jitter, in seconds */
        double m_max_jitter;

	/** Initial period used when m_samples is empty */
	double m_initial_period;

	/** Total number of missing samples */
	int m_missing_samples_total;

	/** number of missing samples recorded in m_samples */
	unsigned int m_missing_samples;

	/** the value of the last index given to us using update */
	int64_t m_last_index;

	/** m_last_index is initialized */
	bool m_have_last_index;

        /** The last time given to updateReference */
        base::Time m_last_reference;

        /** The count of samples that are expected to be lost within
         * expected_loss_timeout calls to update().
         */
        int m_expected_losses;

        /** The count of samples that are expected to be lost within
         * expected_loss_timeout calls to update().
         */
        int m_rejected_expected_losses;

        /** Count of cycles during which m_expected_losses is expected to be
         * valid
         */
        int m_expected_loss_timeout;

        /** Set the base time to the given value. reset_time is used in update()
         * to trigger new updates when necessary
         */
        void resetBaseTime(double new_value, double reset_time);

        /** Internal helper for the reset() methods, that take double directly.
         * This avoid converting the internal parameters to base::Time and then
         * to double again.
         */
	void internalReset(double window,
			   double initial_period,
			   double min_latency,
			   int lost_threshold = 2);

        /** Internal method that pushes a new sample on m_samples while making
         * sure that internal constraints are met (as e.g. that there are no NaN
         * at the beginning of the buffer)
         */
        void pushSample(double time);

    public:
        /** Creates a timestamp estimator
         *
         * @arg window the size of the window that should be used to the
         *        estimation. It should be an order of magnitude smaller
         *        than the period drift in the estimated stream
         *
	 * @arg initial_period initial estimate for the period, used to fill
	 *        the initial window.
	 *
	 * @arg min_latency the smallest amount of latency between the
	 *        reference timestamps and the data timestamps
	 *
         * @arg lost_threshold if that many calls to update() are out of bounds
         *        (i.e. the distance between the two timestamps are greater than
         *        the period), then we consider that we lost samples and update
         *        the timestamp accordingly. Set to 0 if you are sure that the
         *        acquisition latency is lower than the device period.
	 *        Set to INT_MAX if you are sure to either not lose any samples
	 *        or know about all lost samples and use updateLoss()/
	 *        update(base::Time,int)
         */
	TimestampEstimator(base::Time window,
			   base::Time initial_period,
			   base::Time min_latency,
			   int lost_threshold = 2);
	TimestampEstimator(base::Time window,
			   base::Time initial_period,
			   int lost_threshold = 2);
	TimestampEstimator(base::Time window = base::Time(),
			   int lost_threshold = 2);

        /** Resets this estimator to an initial state, reusing the same
         * parameters
         *
         * See the constructor documentation for parameter documentation
         */
	void reset();

        /** @overload
         */
	void reset(base::Time window,
			   int lost_threshold = 2);

        /** @overload
         */
	void reset(base::Time window,
			   base::Time initial_period,
			   int lost_threshold = 2);

        /** Changes the estimator parameters, and resets it to an initial state
         *
         * See the constructor documentation for parameter documentation
         */
	void reset(base::Time window,
			   base::Time initial_period,
			   base::Time min_latency,
			   int lost_threshold = 2);

        /** Updates the estimate and return the actual timestamp for +ts+ */
        base::Time update(base::Time ts);

        /** Updates the estimate and return the actual timestamp for +ts+,
	 *  calculating lost samples from the index
	 */
	base::Time update(base::Time ts, int64_t index);

        /** Updates the estimate for a known lost sample */
	void updateLoss();

        /** Updates the estimate using a reference */
	void updateReference(base::Time ts);

        /** The currently estimated period
         */
        base::Time getPeriod() const;

        /** Shortens the sample list so that the addition of \c time would not
         * overflow the window. Calling this is strongly recommended if there is
         * a chance of only calling updateLoss for long stretches of time
	 */
        void shortenSampleList(double current);

        /** The total estimated count of lost samples so far */
        int getLostSampleCount() const;

        /** Returns true if updateLoss and getPeriod can give valid estimates */
	bool haveEstimate() const;

        /** Returns the current latency estimate. This is valid only if
         * updateReference() is called
         */
        base::Time getLatency() const;

        /** Returns the maximum jitter duration estimated so far
         *
         * It is reset to zero only on reset
         */
        base::Time getMaxJitter() const;

        /** Returns a data structure that represents the estimator's internal
         * status
         *
         * This is constant time
         */
        TimestampEstimatorStatus getStatus() const;

        /** Dumps part of the estimator's internal state to std::cout
         */
        void dumpInternalState() const;
    };
}

#endif

