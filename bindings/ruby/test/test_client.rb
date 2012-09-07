# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestClient < MajordomoTestCase
  def test_init
    client = Majordomo::Client.new(BROKER)
    assert_instance_of Majordomo::Client, client
    client.close

    assert_raises TypeError do
      Majordomo::Client.new(:invalid)
    end

    client = Majordomo::Client.new(BROKER, true)
  ensure
    client.close
  end

  def test_broker
    client = Majordomo::Client.new(BROKER)
    assert_equal BROKER, client.broker
  ensure
    client.close
  end

  def test_timeout
    client = Majordomo::Client.new(BROKER)
    assert_equal 2500, client.timeout
    client.timeout = 1000
    assert_equal 1000, client.timeout
  ensure
    client.close
  end

  def test_close
    client = Majordomo::Client.new(BROKER)
    assert_nil client.close
    assert_raises RuntimeError do
      client.timeout
    end
  end

  def test_send
    client = Majordomo::Client.new(BROKER, true)
    client.timeout = 100;
    assert client.send("test", "message")
  ensure
    client.close
  end
end