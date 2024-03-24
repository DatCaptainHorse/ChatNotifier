module;

#include <string>

#include <imgui.h>
#include <imgui_internal.h>

export module notification;

import effect;
import config;

// Class for notifications
export class Notification {
	std::string m_fullText;
	float m_lifetime = 0.0f;
	const float m_maxLifetime;
	TextEffectMix m_effectMix;

public:
	Notification() = delete;
	~Notification() = default;

	explicit Notification(std::string text)
		: m_fullText(std::move(text)), m_maxLifetime(global_config.notifAnimationLength) {
		m_effectMix.setMixIntensity(global_config.notifEffectIntensity);
		m_effectMix.setMixSpeed(global_config.notifEffectSpeed);
		m_effectMix.set_text(m_fullText);

		// Default mix
		m_effectMix.add_effect<TextEffectFade>(1.0f, 1.0f);
		m_effectMix.add_effect<TextEffectTransition>(1.0f, 1.0f);
		m_effectMix.add_effect<TextEffectWave>(1.0f, 1.0f);
		m_effectMix.add_effect<TextEffectRainbow>(1.0f, 1.0f);
	}

	// Render method
	void render(ImFont *notifFont) {
		// Skip if lifetime is over
		if (is_dead()) return;

		const auto mainMonitor = ImGui::GetViewportPlatformMonitor(ImGui::GetMainViewport());
		ImGui::SetNextWindowPos(mainMonitor->MainPos, ImGuiCond_Once);
		ImGui::SetNextWindowSize(mainMonitor->MainSize, ImGuiCond_Once);

		ImGui::Begin("##notifWindow", nullptr,
					 ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground |
						 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing |
						 ImGuiWindowFlags_NoDocking);

		// Set transparency for window viewport
		const auto viewport = ImGui::GetWindowViewport();
		viewport->Flags |= ImGuiViewportFlags_TransparentClearColor;
		viewport->Flags |= ImGuiViewportFlags_TopMost;
		viewport->Flags |= ImGuiViewportFlags_NoInputs;
		viewport->Flags |= ImGuiViewportFlags_NoFocusOnAppearing;
		viewport->Flags |= ImGuiViewportFlags_NoFocusOnClick;
		viewport->Flags |= ImGuiViewportFlags_NoTaskBarIcon;
		viewport->Flags |= ImGuiViewportFlags_NoDecoration;

		{
			// General time variable
			const auto timeT = m_lifetime / m_maxLifetime;
			m_effectMix.render(notifFont, timeT, TextEffectFlags::eCenteredHorizontal);
		}

		ImGui::End();

		// Update times
		m_lifetime += ImGui::GetIO().DeltaTime;
	}

	// Returns true if notification has lived its lifetime
	[[nodiscard]] auto is_dead() const -> bool { return m_lifetime >= m_maxLifetime; }
};
