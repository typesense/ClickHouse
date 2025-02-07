-- Tags: distributed

SET log_queries=1;

DROP TABLE IF EXISTS times;
CREATE TEMPORARY TABLE times (t DateTime);

INSERT INTO times SELECT now();

SELECT count('special query for 01290_max_execution_speed_distributed')
FROM
(
    SELECT
        sleep(0.001), -- sleep for each block is needed to make sure the query cannot finish too fast,
                      -- i.e. before the first timer tick that might take up to 10ms on some platforms including ARM.
                      -- the timer's resolution is important because we use `CLOCK_MONOTONIC_COARSE` in `ReadProgressCallback`.
                      -- ATST `sleep` uses more accurate timer, so we won't spend more than 100ms in total sleeping.
        number
    FROM remote('127.0.0.{2,3}', numbers(100000))
)
SETTINGS max_execution_speed = 100000, timeout_before_checking_execution_speed = 0, max_block_size = 1000, log_comment='01290_8ca5d52f-8582-4ee3-8674-351c76d67b8c';

INSERT INTO times SELECT now();

SELECT max(t) - min(t) >= 1 FROM times;

-- Check that the query was also throttled on "remote" servers.
SYSTEM FLUSH LOGS;
SELECT COUNT()
FROM system.query_log
WHERE
    current_database = currentDatabase() AND
    event_date >= yesterday() AND
    event_time >= now() - INTERVAL '5 MINUTES' AND -- time limit for tests not marked `long` is 3 minutes, 5 should be more than enough
    log_comment = '01290_8ca5d52f-8582-4ee3-8674-351c76d67b8c' AND
    query LIKE '%special query for 01290_max_execution_speed_distributed%' AND
    query NOT LIKE '%system.query_log%' AND
    type = 'QueryFinish' AND
    query_duration_ms >= 500
SETTINGS max_threads = 0;
