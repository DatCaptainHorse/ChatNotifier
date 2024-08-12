module;

#include <map>
#include <string>
#include <chrono>
#include <memory>
#include <functional>

#include <hv/WebSocketClient.h>

export module twitch;

import config;
import common;
import commands;
import scripting;

constexpr auto twitch_client_id = "ugoh79sz2as94l6dqadpkzilpohdng";
constexpr auto twitch_response_type = "token";
constexpr auto twitch_redirect_uri = "http://localhost:3000";
constexpr auto twitch_scope = "chat:read";

// Enable usage of bitmask operators for CommandCooldownType
consteval void enable_bitmask_operators(CommandCooldownType) {}

// Enum class of connection status
export enum class ConnectionStatus { eDisconnected, eConnecting, eConnected };

// Callback type for Twitch chat message
export using TwitchChatMessageCallback = std::function<void(const TwitchChatMessage &)>;

// Class for handling Twitch chat connection
export class TwitchChatConnector {
	static inline ConnectionStatus m_connStatus;
	static inline hv::WebSocketClient m_client;

	static inline TwitchChatMessageCallback m_onMessage;

	static inline std::string m_oauthToken;

	// Global cooldown time
	static inline std::chrono::time_point<std::chrono::steady_clock> m_lastCommandTime;

public:
	// Initializes the connector resources, with given callback
	static auto initialize(const TwitchChatMessageCallback &onMessage) -> Result {
		m_connStatus = ConnectionStatus::eDisconnected;
		m_onMessage = onMessage;
		return Result();
	}

	// Cleans up resources used by the connector, disconnecting first if connected
	static void cleanup() {
		if (m_connStatus > ConnectionStatus::eDisconnected) disconnect();
	}

	// Connects to the given channel's chat
	static auto connect() -> Result {
		// If already connected or any parameter is empty, return
		if (m_connStatus > ConnectionStatus::eDisconnected) return Result(1, "Already connected");

		// Set to connecting
		m_connStatus = ConnectionStatus::eConnecting;

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
		if (m_connStatus == ConnectionStatus::eDisconnected) return;

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

			const auto chatMsg = TwitchChatMessage(user, chat);
			if (!chatMsg.is_command()) {
				if (!global_users.contains(user)) {
					const auto twUser = std::make_shared<TwitchUser>(user, chatMsg);
					// twUser->userVoice = random_int(0, TTSHandler::get_num_voices() - 1);
					global_users[user] = twUser;
				} else
					global_users[user]->lastMessage = chatMsg;

				return;
			}

			// Check cooldowns (global_config.enabledCooldowns, global_config.cooldownTime)
			// before calling the callback
			if (global_config.enabledCooldowns & CommandCooldownType::eGlobal) {
				const auto now = std::chrono::steady_clock::now();
				if (now - m_lastCommandTime <
					std::chrono::seconds(global_config.cooldownTime.value))
					return;

				m_lastCommandTime = now;
			}
			if (global_config.enabledCooldowns & CommandCooldownType::ePerUser) {
				if (global_users.contains(user)) {
					if (const auto now = std::chrono::steady_clock::now();
						now - global_users[user]->lastMessage.time <
							std::chrono::seconds(global_config.cooldownTime.value) &&
						!global_users[user]->bypassCooldown)
						return;
				}
			}
			if (global_config.enabledCooldowns & CommandCooldownType::ePerCommand) {
				if (!chatMsg.is_command()) return;
				// Get the command from the message
				const auto extractedCommand = chatMsg.get_command();
				const auto cmdLastExec = CommandHandler::get_last_executed_time(
					CommandHandler::get_command_key(extractedCommand));
				if (const auto now = std::chrono::steady_clock::now();
					now - cmdLastExec < std::chrono::seconds(global_config.cooldownTime.value))
					return;
			}

			if (chatMsg.is_command()) {
				if (!global_users.contains(user)) {
					const auto twUser = std::make_shared<TwitchUser>(user, chatMsg);
					// twUser->userVoice = random_int(0, TTSHandler::get_num_voices() - 1);
					global_users[user] = twUser;
				} else
					global_users[user]->lastMessage = chatMsg;
			}

			m_onMessage(chatMsg);
		}
	}

	static void handle_close() { m_connStatus = ConnectionStatus::eDisconnected; }
};
