#include "TimerUtil.h"
#include "HMath.h"
#include <utility>
#include <functional>
#include <algorithm>
#include "../Memory/GameData.h"
#include "../Utils/DrawUtils.h"
using namespace std;
template <typename T>
class AnimationValue {
private:
	T oldValue;
	T currentValue;
	long startTime;
	float duration;
	T defaultValue;
	bool equal(T val) {
		return (float)currentValue == (float)val;
	}
public:
	static_assert(std::is_same<T, float>::value ||
		std::is_same<T, double>::value ||
		std::is_same<T, int>::value ||
		std::is_same<T, MC_Color>::value,
		"Only float, double, and int are supported.");

	std::function<float(float)> ease;

	float progress() {
		float progress = min((int)(TimerUtil::getCurrentMs() - startTime), (int)(duration * 1000.f)) / (duration * 1000.f);
		return isnan(progress) ? 1 : progress;
	}
	T get() {
		return oldValue + (currentValue - oldValue) * ease(progress());
	};
	void set(T val, float duration = 0.5f) {
		if (this->duration != duration || !equal(val)) {
			oldValue = get();
			currentValue = val;
			startTime = TimerUtil::getCurrentMs();
			this->duration = duration;
		}
	};
	void forceSet(T val) {
		oldValue = val;
		currentValue = val;
	};
	void reset() {
		forceSet(defaultValue);
	}
	AnimationValue(T defaultVal, float(*ease)(float) = [](float time) {
		return time;
		}) {
		startTime = TimerUtil::getCurrentMs();
		this->ease = ease;
		defaultValue = defaultVal;
		forceSet(defaultVal);
	};
};
template <>
MC_Color AnimationValue<MC_Color>::get() {
	float prog = ease(progress());
	return MC_Color(
		(oldValue.r + (currentValue.r - oldValue.r) * prog) * 255,
		(oldValue.g + (currentValue.g - oldValue.g) * prog) * 255,
		(oldValue.b + (currentValue.b - oldValue.b) * prog) * 255,
		(oldValue.a + (currentValue.a - oldValue.a) * prog) * 255
	);
}
template <>
bool AnimationValue<MC_Color>::equal(MC_Color val) {
	return currentValue.r == val.r && currentValue.g == val.g && currentValue.b == val.b && currentValue.a == val.a;
}

inline float easeInSine(float x) {
	return 1 - cos((x * PI) / 2);
};
inline float easeOutSine(float x) {
	return sin((x * PI) / 2);
};
inline float easeInOutSine(float x) {
	return -(cos(PI * x) - 1) / 2;
};
inline float easeInQuad(float x) {
	return x * x;
};
inline float easeOutQuad(float x) {
	return 1 - (1 - x) * (1 - x);
};
inline float easeInOutQuad(float x) {
	return x < 0.5 ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2;
};
inline float easeInCubic(float x) {
	return x * x * x;
};
inline float easeOutCubic(float x) {
	return 1 - pow(1 - x, 3);
};
inline float easeInOutCubic(float x) {
	return x < 0.5 ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2;
};
inline float easeInQuart(float x) {
	return x * x * x * x;
};
inline float easeOutQuart(float x) {
	return 1 - pow(1 - x, 4);
};
inline float easeInOutQuart(float x) {
	return x < 0.5 ? 8 * x * x * x * x : 1 - pow(-2 * x + 2, 4) / 2;
};
inline float easeInQuint(float x) {
	return x * x * x * x * x;
};
inline float easeOutQuint(float x) {
	return 1 - pow(1 - x, 5);
};
inline float easeInOutQuint(float x) {
	return x < 0.5 ? 16 * x * x * x * x * x : 1 - pow(-2 * x + 2, 5) / 2;
};
inline float easeInExpo(float x) {
	return x == 0 ? 0 : pow(2, 10 * x - 10);
};
inline float easeOutExpo(float x) {
	return x == 1 ? 1 : 1 - pow(2, -10 * x);
};
inline float easeInOutExpo(float x) {
	return x == 0
		? 0
		: x == 1
		? 1
		: x < 0.5 ? pow(2, 20 * x - 10) / 2
		: (2 - pow(2, -20 * x + 10)) / 2;
};
inline float easeInCirc(float x) {
	return 1 - sqrt(1 - pow(x, 2));
};
inline float easeOutCirc(float x) {
	return sqrt(1 - pow(x - 1, 2));
};
inline float easeInOutCirc(float x) {
	return x < 0.5
		? (1 - sqrt(1 - pow(2 * x, 2))) / 2
		: (sqrt(1 - pow(-2 * x + 2, 2)) + 1) / 2;
};
inline float easeInBack(float x) {
	float c1 = 1.70158;
	float c3 = c1 + 1;

	return c3 * x * x * x - c1 * x * x;
};
inline float easeOutBack(float x) {
	float c1 = 1.70158;
	float c3 = c1 + 1;

	return 1 + c3 * pow(x - 1, 3) + c1 * pow(x - 1, 2);
};
inline float easeInOutBack(float x) {
	float c1 = 1.70158;
	float c2 = c1 * 1.525;

	return x < 0.5
		? (pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
		: (pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
};
inline float easeInElastic(float x) {
	float c4 = (2 * PI) / 3;

	return x == 0
		? 0
		: x == 1
		? 1
		: -pow(2, 10 * x - 10) * sin((x * 10 - 10.75) * c4);
};
inline float easeOutElastic(float x) {
	float c4 = (2 * PI) / 3;

	return x == 0
		? 0
		: x == 1
		? 1
		: pow(2, -10 * x) * sin((x * 10 - 0.75) * c4) + 1;
};
inline float easeInOutElastic(float x) {
	float c5 = (2 * PI) / 4.5;

	return x == 0
		? 0
		: x == 1
		? 1
		: x < 0.5
		? -(pow(2, 20 * x - 10) * sin((20 * x - 11.125) * c5)) / 2
		: (pow(2, -20 * x + 10) * sin((20 * x - 11.125) * c5)) / 2 + 1;
};
inline float easeOutBounce(float x) {
	float n1 = 7.5625;
	float d1 = 2.75;

	if (x < 1 / d1) {
		return n1 * x * x;
	}
	else if (x < 2 / d1) {
		return n1 * (x -= 1.5 / d1) * x + 0.75;
	}
	else if (x < 2.5 / d1) {
		return n1 * (x -= 2.25 / d1) * x + 0.9375;
	}
	else {
		return n1 * (x -= 2.625 / d1) * x + 0.984375;
	}
};
inline float easeInBounce(float x) {
	return 1 - easeOutBounce(1 - x);
};
inline float easeInOutBounce(float x) {
	return x < 0.5
		? (1 - easeOutBounce(1 - 2 * x)) / 2
		: (1 + easeOutBounce(2 * x - 1)) / 2;
};