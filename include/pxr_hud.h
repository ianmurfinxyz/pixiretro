#ifndef _PIXIRETRO_HUD_H_
#define _PIXIRETRO_HUD_H_

//
// Resetting the HUD will reset activation delays and phaseins, it will not reset age, modify
// flash state or hidden state. Meaning if label had a finite lifetime and died before the reset,
// the reset will not bring it back. Further if the label is still alive at the point of reset
// it will not make the label younger and delay its death.
//

#include <vector>
#include <memory>
#include "pxr_color.h"
#include "pxr_vec.h"
#include "pxr_gfx.h"

namespace pxr
{

class HUD
{
public:
	using uid_t = uint32_t;

	static constexpr float IMMORTAL_LIFETIME {0.f};

	class Label
	{
		friend HUD;
	public:
		virtual ~Label() = default;

		virtual void onReset();
		virtual void onUpdate(float dt);
		virtual void onDraw(gfx::ScreenID_t screenid) = 0;

		void startFlashing();
		void stopFlashing();
		bool isFlashing() const {return _isFlashing;}
		void hide() {_isHidden = true;}
		void show() {_isHidden = false;}
		bool isHidden() const {return _isHidden;}
		bool isDead() {return _isDead;}
		uid_t getUid() const {return _uid;}
		void setColor(gfx::Color4u color){_color = color;}
		gfx::Color4u getColor(){return _color;}

	protected:
		Label(
			Vector2f position,
			gfx::Color4u color,
			float activationDelay,
			float lifetime
		);

	private:
		void initialize(const HUD* owner, uid_t uid);
  
	protected:
		const HUD* _owner;
		uid_t _uid;
		Vector2i _position;
		gfx::Color4u _color; 
		long _lastFlashNo;
		float _activationDelay;
		float _activationClock;
		float _age;
		float _lifetime;
		bool _flashState;
		bool _isActive;
		bool _isHidden;
		bool _isFlashing;
		bool _isImmortal;
		bool _isDead;
	};

	class TextLabel final : public Label
	{
		friend HUD;
	public:
		TextLabel(
			Vector2f position,
			gfx::Color4u color,
			float activationDelay,
			float lifetime,
			std::string text,
			bool phaseIn,
			gfx::ResourceKey_t fontKey
		);

		~TextLabel() = default;

		void onReset();
		void onUpdate(float dt);
		void onDraw(gfx::ScreenID_t screenid);

	private:
		gfx::ResourceKey_t _fontKey;
		std::string _fullText;
		std::string _visibleText;
		long _lastPhaseInNo;
		int _nextCharToShow;
		bool _isPhasingIn;
	};

	class IntLabel final : public Label
	{
		friend HUD;
	public:
		IntLabel(
			Vector2f position,
			gfx::Color4u color,
			float activationDelay,
			float lifetime,
			const int& sourceValue,
			int precision,
			gfx::ResourceKey_t fontKey
		);

		~IntLabel() = default;

		void onReset();
		void onUpdate(float dt);
		void onDraw(gfx::ScreenID_t screenid);

	private:
		void composeDisplayStr();

	private:
		gfx::ResourceKey_t _fontKey;
		const int& _sourceValue;
		int _displayValue;
		int _precision;
		std::string _displayStr;
	};

	class BitmapLabel final : public Label
	{
		friend HUD;
	public:
		BitmapLabel(
			Vector2f position,
			gfx::Color4u color,
			float activationDelay,
			float lifetime,
			gfx::ResourceKey_t _sheetKey,
			gfx::SpriteID_t _spriteid,
			bool mirrorX = false,
			bool mirrorY = false
		);

		~BitmapLabel() = default;

		void onReset();
		void onUpdate(float dt);
		void onDraw(gfx::ScreenID_t screenid);

	private:
		gfx::ResourceKey_t _sheetKey;
		gfx::SpriteID_t _spriteid;
		bool _mirrorX;
		bool _mirrorY;
	};

public:
	HUD(float flashPeriod, float phaseInPeriod);

	void onReset();
	void onUpdate(float dt);
	void onDraw(gfx::ScreenID_t screenid);

	uid_t addLabel(std::unique_ptr<Label> label);
	void removeLabel(uid_t uid);
	void clear();

	bool hideLabel(uid_t uid);
	bool showLabel(uid_t uid);
	bool startLabelFlashing(uid_t uid);
	bool stopLabelFlashing(uid_t uid);

	void setFlashPeriod(float period) {_flashPeriod = period;}
	void setPhasePeriod(float period) {_phaseInPeriod = period;}
	long getFlashNo() const {return _flashNo;}
	long getPhaseInNo() const {return _phaseInNo;}

	bool setLabelColor(uid_t uid, gfx::Color4u color);

// returns black if label does not exist.
	gfx::Color4u getLabelColor(uid_t uid); 

private:
	std::vector<std::unique_ptr<Label>>::iterator findLabel(uid_t uid);

private:
	std::vector<std::unique_ptr<Label>> _labels;
	uid_t _nextUid;
	long _flashNo;
	long _phaseInNo;
	float _flashPeriod;
	float _phaseInPeriod;
	float _flashClock;
	float _phaseInClock;
};

} // namespace pxr

#endif
