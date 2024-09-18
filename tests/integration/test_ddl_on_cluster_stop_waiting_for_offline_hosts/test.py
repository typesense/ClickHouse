import pytest
import time

from helpers.cluster import ClickHouseCluster

cluster = ClickHouseCluster(__file__)

node1 = cluster.add_instance(
    "node1",
    main_configs=["configs/remote_servers.xml"],
    with_zookeeper=True,
    stay_alive=True,
)
node2 = cluster.add_instance(
    "node2", main_configs=["configs/remote_servers.xml"], with_zookeeper=True
)
node3 = cluster.add_instance(
    "node3", main_configs=["configs/remote_servers.xml"], with_zookeeper=True
)
node4 = cluster.add_instance(
    "node4", main_configs=["configs/remote_servers.xml"], with_zookeeper=True
)


@pytest.fixture(scope="module")
def started_cluster():
    try:
        cluster.start()
        yield cluster

    finally:
        cluster.shutdown()


def test_stop_waiting_for_offline_hosts(started_cluster):
    timeout = 10
    settings = {"distributed_ddl_task_timeout": timeout}

    start = time.time()
    node1.query(
        "DROP TABLE IF EXISTS test_table ON CLUSTER test_cluster SYNC",
        settings=settings,
    )
    assert time.time() - start < timeout

    start = time.time()
    node1.query(
        "CREATE TABLE test_table ON CLUSTER test_cluster (x Int) Engine=Memory",
        settings=settings,
    )
    assert time.time() - start < timeout

    node4.stop()

    start = time.time()
    with pytest.raises(Exception) as err:
        node1.query(
            "DROP TABLE IF EXISTS test_table ON CLUSTER test_cluster SYNC",
            settings=settings,
        )
    assert "Return code: 159" in str(err.value)
    assert time.time() - start >= timeout

    start = time.time()
    with pytest.raises(Exception) as err:
        node1.query(
            "CREATE TABLE test_table ON CLUSTER test_cluster (x Int) Engine=Memory",
            settings=settings,
        )
    assert "Return code: 159" in str(err.value)
    assert time.time() - start >= timeout

    settings = {
        "distributed_ddl_task_timeout": timeout,
        "distributed_ddl_output_mode": "throw_only_active",
    }

    start = time.time()
    node1.query(
        "DROP TABLE IF EXISTS test_table ON CLUSTER test_cluster SYNC",
        settings=settings,
    )
    assert time.time() - start < timeout

    start = time.time()
    node1.query(
        "CREATE TABLE test_table ON CLUSTER test_cluster (x Int) Engine=Memory",
        settings=settings,
    )
    assert time.time() - start < timeout
