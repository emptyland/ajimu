#ifndef AJIMU_VALUES_REACHABLE_H
#define AJIMU_VALUES_REACHABLE_H

#include "glog/logging.h"

namespace ajimu {
namespace values {
class ObjectManagement;

inline unsigned InvWhite(unsigned white);
inline unsigned White(unsigned white);

class Reachable {
public:
	enum Flags {
		WHITE_BIT0 = 1 << 0,
		WHITE_BIT1 = 1 << 1,
		WHITE_MASK = (WHITE_BIT0 | WHITE_BIT1),
		BLACK = 1 << 2,
	};

	~Reachable() {}

	bool TestInvWhite(unsigned white) const {
		return color_ == InvWhite(white);
	}

	bool TestWhite(unsigned white) const {
		return color_ == White(white);
	}

	bool IsBlack() const {
		return (color_ == BLACK);
	}

	friend class ObjectManagement;
protected:
	Reachable(Reachable *next, unsigned white)
		: next_(next)
		, color_(white) {
		DCHECK(white == WHITE_BIT0 || white == WHITE_BIT1);
	}

private:
	Reachable(const Reachable &) = delete;
	void operator = (const Reachable &) = delete;

	void ToBlack() {
		color_ = BLACK;
	}

	void ToWhite(unsigned white) {
		DCHECK(Reachable::WHITE_BIT0 == white ||
				Reachable::WHITE_BIT1 == white);
		color_ = white;
	}

	Reachable *next_;
	unsigned color_;
}; // class Reachable

inline unsigned InvWhite(unsigned white) {
	DCHECK(Reachable::WHITE_BIT0 == white ||
			Reachable::WHITE_BIT1 == white);

	return (white == Reachable::WHITE_BIT0) ?
		Reachable::WHITE_BIT1 :
		Reachable::WHITE_BIT0;
}

inline unsigned White(unsigned white) {
	DCHECK(Reachable::WHITE_BIT0 == white ||
			Reachable::WHITE_BIT1 == white);
	return white;
}

} // namespace values
} // namespace ajimu

#endif //AJIMU_VALUES_REACHABLE_H

