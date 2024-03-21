module;

#include <map>
#include <chrono>
#include <string>
#include <functional>

#include <hv/WebSocketClient.h>

export module twitch;

import config;
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

	static inline TwitchChatMessageCallback m_onMessage;

	// Keep track of user -> last command time for cooldowns
	static inline std::chrono::time_point<std::chrono::steady_clock> m_lastCommandTime;
	static inline std::map<std::string, std::chrono::time_point<std::chrono::steady_clock>>
		m_lastCommandTimes;

public:
	// Initializes the connector resources, with given callback
	static auto initialize(const TwitchChatMessageCallback &onMessage) -> Result {
		m_connStatus = ConnectionStatus::eDisconnected;
		m_onMessage = onMessage;
		return Result();
	}

	// Cleans up resources used by the connector, disconnecting first if connected
	static void cleanup() {
		if (m_connStatus > ConnectionStatus::eDisconnected)
			disconnect();
	}

	// Connects to the given channel's chat
	static auto connect(const std::string &authToken, const std::string &authUser,
						const std::string &channel) -> Result {
		// If already connected or any parameter is empty, return
		if (m_connStatus > ConnectionStatus::eDisconnected || channel.empty() ||
			authToken.empty() || authUser.empty())
			return Result(1, "Already connected or invalid parameters");

		// Set to connecting
		m_connStatus = ConnectionStatus::eConnecting;

		// Set params
		global_config.twitchAuthToken = authToken;
		global_config.twitchAuthUser = authUser;
		global_config.twitchChannel = channel;

		// Remove extra \0's from config strings (resize causes auth to fail)
		std::erase(global_config.twitchAuthToken, '\0');
		std::erase(global_config.twitchAuthUser, '\0');
		std::erase(global_config.twitchChannel, '\0');

		// Set handlers
		m_client.onopen = handle_open;
		m_client.onmessage = handle_message;
		m_client.onclose = handle_close;

		// Set auto-reconnect settings
		auto reconn = reconn_setting_s();
		m_client.setReconnect(&reconn);

		// Connection making
		if (m_client.open("ws://irc-ws.chat.twitch.tv:80") != 0) {
			m_connStatus = ConnectionStatus::eDisconnected;
			return Result(2, "Failed to open connection");
		}

		return Result();
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
		m_client.send(std::format("PASS oauth:{}\r\n", global_config.twitchAuthToken));
		m_client.send(std::format("NICK {}\r\n", global_config.twitchAuthUser));
		m_connStatus = ConnectionStatus::eConnected;
	}

	static void handle_message(const std::string &msg) {
		// If ":tmi.twitch.tv NOTICE * :Login authentication failed" is found, report error
		if (msg.find(":tmi.twitch.tv NOTICE * :Login authentication failed") != std::string::npos) {
			std::cerr << "Error: Login authentication failed" << std::endl;
			disconnect();
			return;
		}

		// If "PING :tmi.twitch.tv", respond with "PONG :tmi.twitch.tv"
		if (msg.find("PING :tmi.twitch.tv") != std::string::npos) {
			m_client.send("PONG :tmi.twitch.tv\r\n");
			return;
		}

		// If ":tmi.twitch.tv 001" is received, join the channel
		if (msg.find(":tmi.twitch.tv 001") != std::string::npos) {
			m_client.send(std::format("JOIN #{}\r\n", global_config.twitchChannel));
			return;
		}

		// Validate message, then get user and message, passing them to the callback
		if (const std::string message = msg; message.find("PRIVMSG") != std::string::npos) {
			auto user = message.substr(1, message.find('!') - 1);
			// Trim away newlines, carriage returns and tabs
			std::erase(user, '\n');
			std::erase(user, '\r');
			std::erase(user, '\t');

			auto chat = message.substr(message.find(':', 1) + 1);
			// Trim away newlines, carriage returns and tabs
			std::erase(chat, '\n');
			std::erase(chat, '\r');
			std::erase(chat, '\t');

			// Check cooldown (global_config.cooldownType + global_config.cooldownTime) before
			// calling the callback
			switch (global_config.cooldownType) {
			case CommandCooldownType::eGlobal: {
				if (std::chrono::steady_clock::now() - m_lastCommandTime <
					std::chrono::seconds(global_config.cooldownTime))
					return;
				m_lastCommandTime = std::chrono::steady_clock::now();
				break;
			}
			case CommandCooldownType::ePerUser: {
				if (std::chrono::steady_clock::now() - m_lastCommandTimes[user] <
					std::chrono::seconds(global_config.cooldownTime))
					return;
				m_lastCommandTimes[user] = std::chrono::steady_clock::now();
				break;
			case CommandCooldownType::eNone:
			default:
				break;
			}
			}

			m_onMessage({user, chat});
		}
	}

	static void handle_close() { m_connStatus = ConnectionStatus::eDisconnected; }
};
