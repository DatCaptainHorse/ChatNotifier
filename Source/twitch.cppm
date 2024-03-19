module;

#include <hv/WebSocketClient.h>
#include <fmt/format.h>

#include <functional>
#include <string>

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
	static inline ConnectionStatus m_connStatus;
	static inline hv::WebSocketClient m_client;

	static inline std::string m_authToken, m_authUser, m_channel;
	static inline TwitchChatMessageCallback m_onMessage;

public:
	// Initializes the connector resources, with given callback
	static void initialize(const TwitchChatMessageCallback &onMessage) {
		m_connStatus = ConnectionStatus::eDisconnected;
		m_onMessage = onMessage;
	}

	// Cleans up resources used by the connector, disconnecting first if connected
	static void cleanup() {
		if (m_connStatus > ConnectionStatus::eDisconnected)
			disconnect();
	}

	// Connects to the given channel's chat
	static void connect(const std::string &authToken, const std::string &authUser,
						const std::string &channel) {
		// If already connected or any parameter is empty, return
		if (m_connStatus > ConnectionStatus::eDisconnected || channel.empty() ||
			authToken.empty() || authUser.empty())
			return;

		// Set to connecting
		m_connStatus = ConnectionStatus::eConnecting;

		// Set params
		m_authToken = authToken;
		m_authUser = authUser;
		m_channel = channel;

		// Set handlers
		m_client.onopen = handle_open;
		m_client.onmessage = handle_message;
		m_client.onclose = handle_close;

		// Set auto-reconnect settings
		auto reconn = reconn_setting_s();
		reconn.max_retry_cnt = 5;
		m_client.setReconnect(&reconn);

		// Connection making
		if (m_client.open("ws://irc-ws.chat.twitch.tv:80") != 0) {
			fmt::println("Failed to connect to Twitch WS endpoint");
			m_connStatus = ConnectionStatus::eDisconnected;
		}
	}

	// Disconnects existing connection
	static void disconnect() {
		// If not connected, return
		if (m_connStatus == ConnectionStatus::eDisconnected)
			return;

		m_client.close();
		m_connStatus = ConnectionStatus::eDisconnected;
	}

	static auto get_connection_status() { return m_connStatus; }

private: // Handlers
	static void handle_open() {
		m_client.send(fmt::format("PASS oauth:{}\r\n", m_authToken));
		m_client.send(fmt::format("NICK {}\r\n", m_authUser));
		m_client.send(fmt::format("JOIN #{}\r\n", m_channel));
		m_connStatus = ConnectionStatus::eConnected;
	}

	static void handle_message(const std::string &msg) {
		// Validate message, then get user and message, passing them to the callback
		if (const std::string_view message = msg; message.find("PRIVMSG") != std::string::npos) {
			const auto user = message.substr(1, message.find('!') - 1);
			const auto chat = message.substr(message.find(':', 1) + 1);
			m_onMessage({std::string(user), std::string(chat)});
		}
	}

	static void handle_close() { m_connStatus = ConnectionStatus::eDisconnected; }
};
