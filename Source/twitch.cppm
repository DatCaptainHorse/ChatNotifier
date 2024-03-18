module;

#define ASIO_STANDALONE // Standalone Asio, no Boost
#define _WEBSOCKETPP_CPP11_STRICT_ // Force websocket to use C++11

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <fmt/format.h>

#include <thread>

export module twitch;

import common;

// Struct for Twitch message data
export struct TwitchChatMessage {
	std::string user, message;
};

// Enum class of connection status
export enum class ConnectionStatus { eDisconnected, eConnecting, eConnected };

// Callback type for Twitch chat message
export using TwitchChatMessageCallback = std::function<void(const TwitchChatMessage &)>;

// Class for handling Twitch chat connection
export class TwitchChatConnector {
	using client = websocketpp::client<websocketpp::config::asio_client>;
	using message_ptr = websocketpp::config::asio_client::message_type::ptr;

	static inline ConnectionStatus m_connStatus;
	static inline std::thread m_thread;
	static inline std::unique_ptr<client> m_client;

	static inline std::string m_authToken, m_authUser, m_channel;
	static inline TwitchChatMessageCallback m_onMessage;

public:
	// Initializes the connector resources, with given callback
	static void initialize(const TwitchChatMessageCallback &onMessage) {
		m_connStatus = ConnectionStatus::eDisconnected;
		m_onMessage = onMessage;
		m_client = std::make_unique<client>();
	}

	// Cleans up resources used by the connector, disconnecting first if connected
	static void cleanup() {
		if (m_connStatus > ConnectionStatus::eDisconnected)
			disconnect();

		m_client = nullptr;
	}

	// Connects to the given channel's chat
	static void connect(const std::string &authToken, const std::string &authUser,
						const std::string &channel) {
		// If already connected or any parameter is empty, return
		if (m_connStatus > ConnectionStatus::eDisconnected || channel.empty() || authToken.empty() ||
			authUser.empty())
			return;

		// Set to connecting
		m_connStatus = ConnectionStatus::eConnecting;

		// Set params
		m_authToken = authToken;
		m_authUser = authUser;
		m_channel = channel;

		// Disable websocketpp logging
		m_client->clear_access_channels(websocketpp::log::alevel::all);
		m_client->clear_error_channels(websocketpp::log::elevel::all);

		// Init ASIO
		m_client->init_asio();

		// Set handlers
		m_client->set_open_handler(
			[](auto &&PH1) { TwitchChatConnector::handle_open(std::forward<decltype(PH1)>(PH1)); });
		m_client->set_message_handler([](auto &&PH1, auto &&PH2) {
			TwitchChatConnector::handle_message(std::forward<decltype(PH1)>(PH1),
												std::forward<decltype(PH2)>(PH2));
		});
		m_client->set_fail_handler(
			[](auto &&PH1) { TwitchChatConnector::handle_fail(std::forward<decltype(PH1)>(PH1)); });
		m_client->set_close_handler([](auto &&PH1) {
			TwitchChatConnector::handle_close(std::forward<decltype(PH1)>(PH1));
		});

		// Connection making
		websocketpp::lib::error_code err;
		const auto conn = m_client->get_connection("ws://irc-ws.chat.twitch.tv:80", err);
		m_client->connect(conn);

		// Start client run thread
		m_thread = std::thread([] { m_client->run(); });
	}

	// Disconnects existing connection
	static void disconnect() {
		// If not connected, return
		if (m_connStatus == ConnectionStatus::eDisconnected)
			return;

		m_client->stop();
		if (m_thread.joinable())
			m_thread.join();

		// recreate client so it's usable again
		m_client = std::make_unique<client>();

		m_connStatus = ConnectionStatus::eDisconnected;
	}

	static auto get_connection_status() { return m_connStatus; }

private: // Handlers
	static void handle_open(const websocketpp::connection_hdl &hdl) {
		m_client->send(hdl, fmt::format("PASS oauth:{}\r\n", m_authToken),
					   websocketpp::frame::opcode::text);
		m_client->send(hdl, fmt::format("NICK {}\r\n", m_authUser),
					   websocketpp::frame::opcode::text);
		m_client->send(hdl, fmt::format("JOIN #{}\r\n", m_channel),
					   websocketpp::frame::opcode::text);
		m_connStatus = ConnectionStatus::eConnected;
	}

	static void handle_message(const websocketpp::connection_hdl &, const message_ptr &msg) {
		// Validate message, then get user and message, passing them to the callback
		if (const std::string_view message = msg->get_payload();
			message.find("PRIVMSG") != std::string::npos) {
			const auto user = message.substr(1, message.find('!') - 1);
			const auto chat = message.substr(message.find(':', 1) + 1);
			m_onMessage({std::string(user), std::string(chat)});
		}
	}

	static void handle_fail(const websocketpp::connection_hdl &) {
		m_connStatus = ConnectionStatus::eDisconnected;
	}

	static void handle_close(const websocketpp::connection_hdl &) {
		m_connStatus = ConnectionStatus::eDisconnected;
	}
};
