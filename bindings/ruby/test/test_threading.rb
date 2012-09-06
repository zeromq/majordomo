# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestThreading < MajordomoTestCase
  def test_send_recv
    threads = []
    req_received, rep_received = 0, 0
    threads << Thread.new do
      client = Majordomo::Client.new(BROKER, true)
      client.timeout = 100
      100.times do
        client.send("thread_test", "ping")
        reply = client.recv("thread_test")
        rep_received += 1 if reply
      end
    end

    threads << Thread.new do
      worker = Majordomo::Worker.new(BROKER, "thread_test", true)
      100.times do
        request, reply_to = worker.recv
        req_received += 1 if request
        worker.send("pong", reply_to)
      end
    end

    threads.map(&:join)
    assert_equal req_received, 100
    assert_equal rep_received, 100
  end
end
