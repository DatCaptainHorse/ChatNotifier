module;

#include <imgui.h>
#include <imgui_internal.h>

export module effect;

import standard;
import common;

// Text Effect system for text notifications //

// Flags for text effects
export enum class TextEffectFlags { eNone, eCenteredHorizontal };

// Helper methods for rotating text in ImGui - center of rotation is the center of the text
static auto degToRad(const float &deg) -> float { return deg * 0.0174532925f; }

auto operator-(const ImVec2 &l, const ImVec2 &r) -> ImVec2 { return {l.x - r.x, l.y - r.y}; }
auto operator+(const ImVec2 &l, const ImVec2 &r) -> ImVec2 { return {l.x + r.x, l.y + r.y}; }
auto operator*(const ImVec2 &l, const float &r) -> ImVec2 { return {l.x * r, l.y * r}; }
auto operator-=(ImVec2 &l, const ImVec2 &r) -> ImVec2 & {
	l.x -= r.x;
	l.y -= r.y;
	return l;
}
auto operator+=(ImVec2 &l, const ImVec2 &r) -> ImVec2 & {
	l.x += r.x;
	l.y += r.y;
	return l;
}
auto operator*=(ImVec2 &l, const float &r) -> ImVec2 & {
	l.x *= r;
	l.y *= r;
	return l;
}

int g_rotationIndex = 0;
auto RotateBegin() -> void { g_rotationIndex = ImGui::GetWindowDrawList()->VtxBuffer.Size; }
auto RotateEnd(const float &deg) -> void {
	ImVec2 l(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
		u(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
	auto buf = ImGui::GetWindowDrawList()->VtxBuffer;
	for (int i = g_rotationIndex; i < buf.Size; i++)
		l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

	const float cos_a = std::cos(degToRad(deg)), sin_a = std::sin(degToRad(deg));
	const auto center = ImVec2((l.x + u.x) / 2.0f, (l.y + u.y) / 2.0f);
	for (int i = g_rotationIndex; i < buf.Size; i++)
		buf[i].pos = ImRotate(buf[i].pos, cos_a, sin_a) - center;
}

// Method for multiplying two ImVec4 colors
auto multiply_colors(const ImVec4 &c1, const ImVec4 &c2) -> ImVec4 {
	return ImVec4(c1.x * c2.x, c1.y * c2.y, c1.z * c2.z, c1.w * c2.w);
}

// CharacterData struct, holds data for text characters
export struct CharacterData {
	const std::string letter;					  //< Letter of the character
	std::optional<float> offsetY = std::nullopt;  //< Additional offset of the character
	std::optional<float> offsetX = std::nullopt;  //< Additional offset of the character
	std::optional<float> sizeX = std::nullopt;	  //< Size override of the character
	std::optional<float> sizeY = std::nullopt;	  //< Size override of the character
	std::optional<ImVec4> color = std::nullopt;	  //< Color override of the character
	std::optional<float> rotation = std::nullopt; //< Separate rotation of the character

	CharacterData() = delete;
	explicit CharacterData(const std::string &letterStr) : letter(letterStr) {}

	auto apply(const ImVec4 &upperColor, const TextEffectFlags &flags) const -> void {
		if (rotation) RotateBegin();
		if (color) ImGui::PushStyleColor(ImGuiCol_Text, multiply_colors(*color, upperColor));
		const auto vtxBufIdx = ImGui::GetWindowDrawList()->VtxBuffer.Size;

		// Create data for the character
		ImGui::TextUnformatted(letter.c_str());

		// Modify vtx buffer directly to apply effects
		auto &vtxBuf = ImGui::GetWindowDrawList()->VtxBuffer;
		const auto &letterSize = ImGui::CalcTextSize(letter.c_str());
		for (int i = vtxBufIdx; i < vtxBuf.Size; i++) {
			if (offsetX) vtxBuf[i].pos.x += *offsetX;
			if (offsetY) vtxBuf[i].pos.y += *offsetY;
			if (sizeX) vtxBuf[i].pos.x *= *sizeX / letterSize.x;
			if (sizeY) vtxBuf[i].pos.y *= *sizeY / letterSize.y;
		}

		if (rotation) RotateEnd(*rotation);
		if (color) ImGui::PopStyleColor();
	}
};

// TextEffectData struct, holds data for text effects that is passed between effects
export struct TextEffectData {
	std::string text;							   //< Text of the effect
	std::vector<CharacterData> characters;		   //< Characters of the text
	ImFont *font = nullptr;						   //< Main font of the text
	std::optional<ImVec2> position = std::nullopt; //< Position of the text from the top left corner
	std::optional<ImVec2> size = std::nullopt;	   //< Main size of the text
	std::optional<ImVec4> color = std::nullopt;	   //< Main color of the text
	std::optional<float> rotation = std::nullopt;  //< Main rotation of the text

	// Run data
	ImVec2 cursorPos = ImVec2(0.0f, 0.0f);
	ImVec2 textSize = ImVec2(0.0f, 0.0f);

	TextEffectData() = delete;
	explicit TextEffectData(const std::string &text) { set_text(text); }

	auto set_text(const std::string &textstr) -> void {
		text = textstr;
		characters.clear();
		const auto letters = get_letters_mb(text);
		characters.reserve(letters.size());
		for (const auto &letter : letters) characters.emplace_back(letter);
	}

	auto apply(const ImVec2 &rootPos, const TextEffectFlags &flags) const -> void {
		if (characters.empty()) return;
		if (rotation) RotateBegin();
		if (font != nullptr) ImGui::PushFont(font);
		const auto &textSize = ImGui::CalcTextSize(text.c_str());

		// If flag is centered, center this text horizontally
		if (flags == TextEffectFlags::eCenteredHorizontal) {
			const auto windowWidth = ImGui::GetWindowWidth();
			const auto textX = windowWidth / 2.0f - textSize.x / 2.0f;
			ImGui::SetCursorPosX(rootPos.x + textX);
		}

		const auto vtxBufIdx = ImGui::GetWindowDrawList()->VtxBuffer.Size;
		for (int i = 0; i < characters.size(); i++) {
			characters[i].apply(color.value_or(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), flags);
			if (i < characters.size() - 1) ImGui::SameLine(0.0f, is_letters(text) ? -1.0f : 0.0f);
		}

		// Same here, buffer modification but for the whole text
		auto &vtxBuf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = vtxBufIdx; i < vtxBuf.Size; i++) {
			if (position) vtxBuf[i].pos += *position;
			if (size) {
				vtxBuf[i].pos.x = vtxBuf[i].pos.x * size->x / textSize.x;
				vtxBuf[i].pos.y = vtxBuf[i].pos.y * size->y / textSize.y;
			}
		}

		if (font != nullptr) ImGui::PopFont();
		if (rotation) RotateEnd(*rotation);

		// Reduce y offset to make the spaces between lines smaller
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - textSize.y * 0.25f);
	}
};

// MixData struct
export struct MixData {
	float speed = 1.0f, intensity = 1.0f;
};

// TextEffect class, base class for text effects
export class TextEffect {
protected:
	float m_speed = 1.0f;
	float m_intensity = 1.0f;

	virtual ~TextEffect() = default;
	explicit TextEffect(const float &speed, const float &intensity)
		: m_speed(speed), m_intensity(intensity) {}

public:
	// Run method, runs the effect on the text
	// time should be 0.0f -> 1.0f, 0.0f being the start of the effect and 1.0f being the end
	virtual auto run(const TextEffectData &effectData, const MixData &mixData, const float &time)
		-> TextEffectData = 0;
};

// TextEffectMix class, makes multiple text effects run after each other
export class TextEffectMix {
	MixData m_mixData;
	std::string m_fullText;
	std::vector<TextEffectData> m_textLines = {};
	std::vector<std::shared_ptr<TextEffect>> m_effects = {};

public:
	TextEffectMix() = default;
	~TextEffectMix() = default;

	template <typename... Effects>
	explicit TextEffectMix(Effects &&...effects) {
		(m_effects.push_back(std::forward<Effects>(effects)), ...);
	}

	auto setMixSpeed(const float &speed) -> void { m_mixData.speed = speed; }
	auto setMixIntensity(const float &intensity) -> void { m_mixData.intensity = intensity; }

	auto set_text(const std::string &text) -> void {
		m_fullText = text;
		m_textLines.clear();
		// Split text into lines
		const auto lines = split_string(m_fullText, "\n");
		m_textLines.reserve(lines.size());
		for (const auto &line : lines) m_textLines.emplace_back(line);
	}

	auto render(ImFont *mainFont, const float &time,
				const TextEffectFlags &flags = TextEffectFlags::eNone) const -> void {
		if (mainFont != nullptr) ImGui::PushFont(mainFont);
		constexpr auto rootPos = ImVec2(0.0f, 0.0f);
		ImGui::SetCursorPosY(rootPos.y);
		for (int i = 0; i < m_textLines.size(); i++) {
			auto line = m_textLines[i];
			line.cursorPos = ImGui::GetCursorPos();
			line.textSize = ImGui::CalcTextSize(m_fullText.c_str());
			for (const auto &effect : m_effects) line = effect->run(line, m_mixData, time);
			line.apply(rootPos, flags);
		}
		if (mainFont != nullptr) ImGui::PopFont();
	}

	auto clear() -> void { m_textLines.clear(); }

	auto clear_effects() -> void { m_effects.clear(); }

	auto add_effect(const std::shared_ptr<TextEffect> &effect) -> void {
		m_effects.push_back(effect);
	}

	// Template method for adding effects
	template <typename Effect, typename... Args>
	auto add_effect(Args &&...args) -> void {
		m_effects.push_back(std::make_shared<Effect>(std::forward<Args>(args)...));
	}
};

// TextEffectWave, makes a rolling wave with characters
export class TextEffectWave final : public TextEffect {
public:
	~TextEffectWave() override = default;
	explicit TextEffectWave(const float &speed, const float &intensity)
		: TextEffect(speed, intensity) {}

	auto run(const TextEffectData &effectData, const MixData &mixData, const float &time)
		-> TextEffectData override {
		auto newEffectData = effectData;

		const auto totalSpeed = mixData.speed * m_speed;
		const auto totalIntensity = mixData.intensity * m_intensity;

		const auto waveTime = std::fmod(time * totalSpeed, 1.0f);
		const auto waveTimeOffset = waveTime * 2.0f * std::numbers::pi;

		for (auto &character : newEffectData.characters) {
			const auto index = std::distance(newEffectData.characters.data(), &character);
			character.offsetY =
				std::sin(waveTimeOffset + index) * std::numbers::pi * totalIntensity * 2.0f;
		}

		return newEffectData;
	}
};

// Helper method for creating ImVec4 colors from HSV values
auto HSVtoImVec4(const float &h, const float &s, const float &v) -> ImVec4 {
	float r, g, b;
	ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
	return ImVec4(r, g, b, 1.0f);
}

// TextEffectRainbow, makes the text rainbow colored
export class TextEffectRainbow final : public TextEffect {
public:
	~TextEffectRainbow() override = default;
	explicit TextEffectRainbow(const float &speed, const float &intensity)
		: TextEffect(speed, intensity) {}

	auto run(const TextEffectData &effectData, const MixData &mixData, const float &time)
		-> TextEffectData override {
		auto newEffectData = effectData;

		const auto totalSpeed = mixData.speed * m_speed;

		const auto hue = std::fmod(time * totalSpeed, 1.0f);

		for (auto &character : newEffectData.characters) {
			const auto index = std::distance(newEffectData.characters.data(), &character);
			const auto offset = std::fmod(hue + index * 0.05f, 1.0f);
			character.color = HSVtoImVec4(offset, 1.0f, 1.0f);
		}

		return newEffectData;
	}
};

// Transition effect of scrolling past the screen
export class TextEffectTransition final : public TextEffect {
public:
	~TextEffectTransition() override = default;
	explicit TextEffectTransition(const float &speed, const float &intensity)
		: TextEffect(speed, intensity) {}

	auto run(const TextEffectData &effectData, const MixData &mixData, const float &time)
		-> TextEffectData override {
		auto newEffectData = effectData;

		const auto transitionY = std::lerp(-effectData.textSize.y, ImGui::GetWindowHeight(), time);
		newEffectData.position = ImVec2(0.0f, transitionY);

		return newEffectData;
	}
};

// Transition fade effect, fades the text in and out
export class TextEffectFade final : public TextEffect {
public:
	~TextEffectFade() override = default;
	explicit TextEffectFade(const float &speed, const float &intensity)
		: TextEffect(speed, intensity) {}

	auto run(const TextEffectData &effectData, const MixData &mixData, const float &time)
		-> TextEffectData override {
		auto newEffectData = effectData;

		if (time < 0.1f) {
			// Fade-in
			const auto alpha = std::lerp(0.0f, 1.0f, time * 10.0f);
			newEffectData.color = ImVec4(1.0f, 1.0f, 1.0f, alpha);
		} else if (time > 0.9f) {
			// Fade-out
			const auto alpha = std::lerp(1.0f, 0.0f, (time - 0.9f) * 10.0f);
			newEffectData.color = ImVec4(1.0f, 1.0f, 1.0f, alpha);
		}

		return newEffectData;
	}
};
