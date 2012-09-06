# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestWorker < MajordomoTestCase
  def test_init
    worker = Majordomo::Worker.new(BROKER, "test_init")
    assert_instance_of Majordomo::Worker, worker
    worker.close

    assert_raises TypeError do
      Majordomo::Worker.new(:invalid, "test")
    end

    assert_raises TypeError do
      Majordomo::Worker.new(BROKER, :invalid)
    end

    worker = Majordomo::Worker.new(BROKER, "test", true)
  ensure
    worker.close
  end

  def test_broker
    worker = Majordomo::Worker.new(BROKER, "test")
    assert_equal BROKER, worker.broker
  ensure
    worker.close
  end

  def test_service
    service = "test_service"
    worker = Majordomo::Worker.new(BROKER, service)
    assert_equal service, worker.service
  ensure
    worker.close
  end

  def test_heartbeat
    worker = Majordomo::Worker.new(BROKER, "test_heartbeat")
    assert_equal 2500, worker.heartbeat
    worker.heartbeat = 100
    assert_equal 100, worker.heartbeat
  ensure
    worker.close
  end

  def test_reconnect
    worker = Majordomo::Worker.new(BROKER, "test_reconnect")
    assert_equal 2500, worker.reconnect
    worker.reconnect = 100
    assert_equal 100, worker.reconnect
  ensure
    worker.close
  end

  def test_close
    worker = Majordomo::Worker.new(BROKER, "test_close")
    assert_nil worker.close

    assert_raises RuntimeError do
      worker.reconnect
    end
  end

  def test_recv
    service = "test_revc"
    worker = Majordomo::Worker.new(BROKER, service, true)
    worker.heartbeat = 100
    worker.reconnect = 100

    client = Majordomo::Client.new(BROKER, true)
    client.timeout = 100
    assert client.send service, "request"

    request = worker.recv
    assert_instance_of Array, request
    assert_equal "request", request[0]

    assert worker.send("reply", request[1])
    assert_equal "reply", client.recv(service)
  ensure
    client.close
    worker.close
  end
end