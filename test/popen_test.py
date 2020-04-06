import pexpect
import asyncio


all_children = []

async def wait_for(child, pattern):
    i = await child.expect(
        [
            pattern,
            pexpect.TIMEOUT,
            pexpect.EOF
        ],
        timeout=60,
        async_=True
    )
    if i is not 0:
        print("command %s did not see pattern %s before timeout" % (''.join(child.args), pattern))
        import pdb; pdb.set_trace()
        return False
    print("command %s DID pattern %s before timeout" % (''.join(child.args), pattern))
    return True

def start(command):
    child = pexpect.spawn(command, maxread=0)
    all_children.append(child)
    return child

async def main():
    try:
        child1 = start("./kafka-2.4.0-src/bin/zookeeper-server-start.sh ./kafka-2.4.0-src/config/zookeeper.properties")
        assert await wait_for(child1, r'.*binding to port.*')

        child2 = start("./kafka-2.4.0-src/bin/kafka-server-start.sh ./kafka-2.4.0-src/config/server.properties")
        assert await wait_for(child2, r'.*started \(kafka.server.KafkaServer.*')

        await asyncio.sleep(7000)
        # child3 = start("bin/kafka-topics.sh --create --bootstrap-server localhost:9092 --replication-factor 1 --partitions 1 --topic test")
    finally:
        for child in all_children:
            child.close()

if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    loop.run_until_complete(main())
