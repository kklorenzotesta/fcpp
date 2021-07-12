// Copyright © 2021 Giorgio Audrito. All Rights Reserved.

/**
 * @file os.hpp
 * @brief Abstract functions defining an OS interface.
 */

#ifndef FCPP_DEPLOYMENT_OS_H_
#define FCPP_DEPLOYMENT_OS_H_

#include <cassert>

#include <algorithm>
#include <functional>
#include <vector>

#include "lib/settings.hpp"
#include "lib/common/algorithm.hpp"
#include "lib/common/mutex.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {


//! @brief Type for raw messages received.
struct message_type {
    //! @brief Timestamp of message receival.
    times_t time;
    //! @brief UID of the sender device.
    device_t device;
    //! @brief An estimate of the signal power (RSSI).
    real_t power;
    //! @brief The message content (empty content represent no message).
    std::vector<char> content;
};


//! @brief Namespace containing OS-dependent functionalities.
namespace os {


//! @brief Access the local unique identifier.
device_t uid();


/**
 * @brief Low-level interface for hardware network capabilities.
 *
 * It should have the following minimal public interface:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * struct data_type;                            // default-constructible type for settings
 * data_type data;                              // network settings
 * transceiver(data_type);                      // constructor with settings
 * bool send(device_t, std::vector<char>, int); // broadcasts a message after given attemps
 * message_type receive(int);                   // listens for messages after given failed sends
 * ~~~~~~~~~~~~~~~~~~~~~~~~~
 */
struct transceiver;


/**
 * @brief Higher-level interface for network capabilities.
 *
 * @param push Whether incoming messages should be immediately pushed to the node.
 * @param node_t The node type. It has the following method that can be called at any time:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * void receive(message_type&);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~
 * @param transceiver_t The transceiver type.
 */
template <bool push, typename node_t, typename transceiver_t = transceiver>
class network {
  public:
    //! @brief Default-constructible type for network settings.
    using data_type = typename transceiver_t::data_type;

    //! @brief Constructor with default settings.
    network(node_t& n) : m_node(n), m_transceiver({})
        #ifndef FCPP_DISABLE_THREADS
        , m_manager(std::mem_fn(&network::manage), this)
        #endif
        {}

    //! @brief Constructor with given settings.
    network(node_t& n, data_type d) : m_node(n), m_transceiver(d)
        #ifndef FCPP_DISABLE_THREADS
        , m_manager(std::mem_fn(&network::manage), this)
        #endif
        {}

    ~network() {
        m_running = false;
        #ifndef FCPP_DISABLE_THREADS
        m_manager.join();
        #endif
    }

    //! @brief Access to network settings.
    data_type& data() {
        return m_transceiver.data;
    }

    //! @brief Const access to network settings.
    data_type const& data() const {
        return m_transceiver.data;
    }

    //! @brief Schedules the broadcast of a message.
    void send(std::vector<char> m) {
        common::lock_guard<true> l(m_send_mutex);
        m_send = std::move(m);
        m_send_time = m_node.net.internal_time();
        m_attempt = 0;
    }

    //! @brief Retrieves the collection of incoming messages.
    std::vector<message_type> receive() {
        assert(not push);
        std::vector<message_type> m;
        common::lock_guard<true> l(m_receive_mutex);
        std::swap(m, m_receive);
        return m;
    }

  #ifndef FCPP_DISABLE_THREADS
  private:
  #endif
    //! @brief Manages the send and receive of messages.
    void manage() {
        #ifndef FCPP_DISABLE_THREADS
        while (m_running) {
        #endif
            if (not m_send.empty()) {
                common::lock_guard<true> l(m_send_mutex);
                m_send.push_back((char)std::min((m_node.net.internal_time() - m_send_time)*128, times_t{255}));
                // sending
                if (m_transceiver.send(m_node.uid, m_send, m_attempt))
                    m_send.clear();
                else {
                    m_send.pop_back();
                    ++m_attempt;
                }
            }
            #ifndef FCPP_DISABLE_THREADS
            std::this_thread::yield();
            #endif
            // receiving
            message_type m = m_transceiver.receive(m_attempt);
            if (not m.content.empty()) {
                m.time = m_node.net.internal_time() - m.content.back() / times_t{128};
                m.content.pop_back();
                if (push) m_node.receive(m);
                else {
                    common::lock_guard<true> l(m_receive_mutex);
                    m_receive.push_back(std::move(m));
                }
            }
        #ifndef FCPP_DISABLE_THREADS
            std::this_thread::yield();
        }
        #endif
    }

  #ifdef FCPP_DISABLE_THREADS
  private:
  #endif

    //! @brief Reference to the node object.
    node_t& m_node;

    //! @brief Low-level hardware interface.
    transceiver_t m_transceiver;

    //! @brief A mutex for regulating network operations.
    common::mutex<true> m_send_mutex, m_receive_mutex;

    //! @brief Whether the object is alive and running.
    bool m_running = true;

    //! @brief Collection of received messages.
    std::vector<message_type> m_receive;

    //! @brief Message to be sent.
    std::vector<char> m_send;

    //! @brief The internal time of the message to be sent.
    times_t m_send_time;

    //! @brief Number of attempts failed for a send.
    int m_attempt = 0;

    #ifndef FCPP_DISABLE_THREADS
    //! @brief Thread managing send and receive of messages.
    std::thread m_manager;
    #endif
};


}


}

#endif // FCPP_DEPLOYMENT_OS_H_
