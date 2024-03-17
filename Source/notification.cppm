module;

#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

export module notification;

// Class for notifications
export class Notification {
	std::string m_fullText;
	float m_lifetime = 0.0f;
	const float m_maxLifetime;
	float m_effectTime = 0.0f;
	float m_effectSpeedMul = 1.0f;
	std::vector<std::string> m_textLines;

public:
	Notification() = delete;
	~Notification() = default;

	Notification(std::string text, const float &maxLifetime, const float &programTime,
				 const float &effectSpeedMultiplier)
		: m_fullText(std::move(text)), m_maxLifetime(maxLifetime), m_effectTime(programTime),
		  m_effectSpeedMul(effectSpeedMultiplier) {
		// Split the text by newlines
		auto start = 0U;
		auto end = m_fullText.find('\n');
		while (end != std::string::npos) {
			m_textLines.push_back(m_fullText.substr(start, end - start));
			start = end + 1;
			end = m_fullText.find('\n', start);
		}
		m_textLines.push_back(m_fullText.substr(start, end));
	}

	// Render method
	void render(ImFont *notifFont) {
		// Skip if lifetime is over
		if (is_dead())
			return;

		// Set the font now
		ImGui::PushFont(notifFont);

		// General time variable
		const auto timeT = m_lifetime / m_maxLifetime;

		// Rainbows!
		// TODO: Allow multiple effects in future
		const auto rainbow = [this](const float phase) -> ImVec4 {
			const auto red = std::sin(0.1f + 0 + phase) * 0.5f + 0.5f;
			const auto green = std::sin(0.1f + 2 + phase) * 0.5f + 0.5f;
			const auto blue = std::sin(0.1f + 4 + phase) * 0.5f + 0.5f;
			return {red, green, blue, 1.0f};
		};

		// Display the text by lines, to allow for rainbow colors
		for (std::size_t i = 0; i < m_textLines.size(); ++i) {
			const auto line = m_textLines[i];
			auto rainbowed = rainbow(m_effectTime + static_cast<float>(i) * 0.5f);
			// Alpha to fade in when timeT is below 0.1f AND fade back to nothing when timeT is
			// above 0.9f
			rainbowed.w = timeT < 0.1f ? timeT * 10.0f : (1.0f - timeT) * 10.0f;

			ImGui::PushStyleColor(ImGuiCol_Text, rainbowed);

			const auto textSize = ImGui::CalcTextSize(line.c_str());
			// Center text horizontally
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - textSize.x / 2);

			const auto totalTextHeight = textSize.y / 1.5f * static_cast<float>(m_textLines.size());
			const auto lineY = textSize.y / 1.5f * static_cast<float>(i);

			// Animate appearing from top, fading away at center
			const auto offset =
				std::lerp(-totalTextHeight / 2.0f,
						  ImGui::GetWindowHeight() / 2.0f - totalTextHeight / 2.0f, timeT) +
				lineY;

			// Set the cursor position
			ImGui::SetCursorPosY(offset);

			// Render the line
			ImGui::TextUnformatted(line.c_str());

			ImGui::PopStyleColor();
		}

		ImGui::PopFont();

		// Update times
		m_lifetime += ImGui::GetIO().DeltaTime;
		m_effectTime += ImGui::GetIO().DeltaTime * m_effectSpeedMul;
	}

	// Returns true if notification has lived its lifetime
	[[nodiscard]] auto is_dead() const -> bool { return m_lifetime >= m_maxLifetime; }
};
