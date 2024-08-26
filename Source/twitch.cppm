module;

#include <hv/requests.h>
#include <hv/HttpServer.h>
#include <hv/WebSocketClient.h>

export module twitch;

import standard;
import types;
import config;
import common;
import commands;

#ifndef TWITCH_CLIENT_SECRET
#define TWITCH_CLIENT_SECRET "undefined"
#endif

constexpr auto twitch_client_id = "ugoh79sz2as94l6dqadpkzilpohdng";
constexpr auto twitch_redirect_uri = "http://localhost:42069/authchatnotifier";
// Pre-URL encoded chat:read scope
constexpr auto twitch_scope = "chat%3Aread";

// Enable usage of bitmask operators for CommandCooldownType
// consteval void enable_bitmask_operators(CommandCooldownType) {}

std::map<std::string, std::shared_ptr<TwitchUser>> global_users;

// Enum class of connection status
export enum class ConnectionStatus { eDisconnected, eConnecting, eConnected, eError };

// Callback type for Twitch chat message
export using TwitchChatMessageCallback = std::function<void(const TwitchChatMessage &)>;

// Class for handling Twitch chat connection
export class TwitchChatConnector {
	static inline ConnectionStatus m_connStatus;
	static inline hv::WebSocketClient m_client;

	static inline TwitchChatMessageCallback m_onMessage;

	static inline std::string m_oauthCode, m_oauthToken;

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

		// Check if we have a global_config.refreshToken to use
		if (!global_config.refreshToken.empty()) {
			if (!oauth_refresh()) {
				// Attempt full
				if (const auto res = oauth_full(); !res) {
					m_connStatus = ConnectionStatus::eError;
					return res;
				}
			}
		} else if (const auto res = oauth_full(); !res) {
			m_connStatus = ConnectionStatus::eError;
			return res;
		}

		// Set handlers
		m_client.onopen = handle_open;
		m_client.onmessage = handle_message;
		m_client.onclose = handle_close;

		// Set auto-reconnect settings
		auto reconn = reconn_setting_s();
		m_client.setReconnect(&reconn);

		// Connection making
		if (m_client.open("ws://irc-ws.chat.twitch.tv:80") != 0) {
			m_connStatus = ConnectionStatus::eError;
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
		m_client.send(std::format("PASS oauth:{}\r\n", m_oauthToken));
		// as channel is the same as the username of owner usually, we can use it
		m_client.send(std::format("NICK {}\r\n", global_config.twitchChannel));
		m_connStatus = ConnectionStatus::eConnected;
	}

	static auto handle_message(const std::string &msg) -> Result {
		// If ":tmi.twitch.tv NOTICE * :Login authentication failed" is found, report error
		if (msg.find(":tmi.twitch.tv NOTICE * :Login authentication failed") != std::string::npos) {
			disconnect();
			return Result(3, "Login authentication failed");
		}

		// If "PING :tmi.twitch.tv", respond with "PONG :tmi.twitch.tv"
		if (msg.find("PING :tmi.twitch.tv") != std::string::npos) {
			m_client.send("PONG :tmi.twitch.tv\r\n");
			return Result();
		}

		// If ":tmi.twitch.tv 001" is received, join the channel
		if (msg.find(":tmi.twitch.tv 001") != std::string::npos) {
			m_client.send(std::format("JOIN #{}\r\n", global_config.twitchChannel));
			return Result();
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

				return Result();
			}

			// Check cooldowns (global_config.enabledCooldowns, global_config.cooldownTime)
			// before calling the callback
			if (global_config.enabledCooldowns & CommandCooldownType::eGlobal) {
				const auto now = std::chrono::steady_clock::now();
				if (now - m_lastCommandTime <
					std::chrono::seconds(global_config.cooldownTime.value))
					return Result();

				m_lastCommandTime = now;
			}
			if (global_config.enabledCooldowns & CommandCooldownType::ePerUser) {
				if (global_users.contains(user)) {
					if (const auto now = std::chrono::steady_clock::now();
						now - global_users[user]->lastMessage.time <
							std::chrono::seconds(global_config.cooldownTime.value) &&
						!global_users[user]->bypassCooldown)
						return Result();
				}
			}
			if (global_config.enabledCooldowns & CommandCooldownType::ePerCommand) {
				if (!chatMsg.is_command()) return Result();
				// Get the command from the message
				const auto extractedCommand = chatMsg.get_command();
				const auto cmdLastExec = CommandHandler::get_last_executed_time(
					CommandHandler::get_command_key(extractedCommand));
				if (const auto now = std::chrono::steady_clock::now();
					now - cmdLastExec < std::chrono::seconds(global_config.cooldownTime.value))
					return Result();
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

		return Result();
	}

	static void handle_close() { m_connStatus = ConnectionStatus::eDisconnected; }

	static auto oauth_refresh() -> Result {
		// Just post to https://id.twitch.tv/oauth2/token with refresh_token
		const auto tokenUrl =
			std::format("https://id.twitch.tv/oauth2/token?client_id={}&client_secret={}&"
						"refresh_token={}&grant_type=refresh_token",
						twitch_client_id, TWITCH_CLIENT_SECRET, global_config.refreshToken);

		if (const auto resp = requests::post(tokenUrl.c_str()); !resp)
			return Result(2, "Failed to get OAuth token");
		else {
			// Get the token and refresh token from the response
			const auto json = resp->GetJson();
			if (json.contains("access_token"))
				m_oauthToken = json["access_token"].get<std::string>();
			else
				return Result(3, "Failed to get OAuth token from json");

			if (json.contains("refresh_token"))
				global_config.refreshToken = json["refresh_token"].get<std::string>();
		}
		return Result();
	}

	static auto oauth_full() -> Result {
		// Setup server for OAuth code
		hv::HttpService router;
		std::binary_semaphore sem(0);
		router.GET("/authchatnotifier", [&sem](HttpRequest *req, HttpResponse *resp) {
			if (req->query_params.contains("code")) {
				m_oauthCode = req->query_params["code"];
				sem.release();
				return 200;
			} else {
				sem.release();
				return 400;
			}
		});

		hv::HttpServer server(&router);
		server.setHost("localhost");
		server.setPort(42069);
		server.start();

		// Open browser to get OAuth token
		const auto url =
			std::format("https://id.twitch.tv/oauth2/"
						"authorize?response_type=code&client_id={}&redirect_uri={}&scope={}",
						twitch_client_id, twitch_redirect_uri, twitch_scope);

		// Open browser, based on OS ifdef's
#if defined(_WIN32)
		system(std::format("explorer \"{}\"", url).c_str());
#elif defined(__APPLE__)
		system(std::format("open \"{}\"", url).c_str());
#elif defined(__linux__)
		system(std::format("xdg-open \"{}\"", url).c_str());
#endif

		// Wait for semaphore to be released
		sem.acquire();
		server.stop();

		// Check if we have a code
		if (m_oauthCode.empty()) return Result(2, "Failed to get OAuth code");

		// Get OAuth token from https://id.twitch.tv/oauth2/token
		const auto tokenUrl =
			std::format("https://id.twitch.tv/oauth2/token?client_id={}&client_secret={}&code={}&"
						"grant_type=authorization_code&redirect_uri={}",
						twitch_client_id, TWITCH_CLIENT_SECRET, m_oauthCode, twitch_redirect_uri);

		if (const auto resp = requests::post(tokenUrl.c_str()); !resp)
			return Result(4, "Failed to get OAuth token");
		else {
			// Get the token and refresh token from the response
			const auto json = resp->GetJson();
			if (json.contains("access_token"))
				m_oauthToken = json["access_token"].get<std::string>();
			else
				return Result(5, "Failed to get OAuth token from json");

			if (json.contains("refresh_token"))
				global_config.refreshToken = json["refresh_token"].get<std::string>();
		}
		return Result();
	}
};
