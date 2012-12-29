#include "macro_analyzer.h"
#include "object_management.h"
#include "object.h"
#include <stdio.h>

namespace ajimu {
namespace vm {

using values::ObjectManagement;
using values::Object;

#define Null (obm_->Constant(values::kEmptyList))
#define Kof(i) (obm_->Constant(values::k##i))
Object *MacroAnalyzer::Extend(Object *s, Object *o) {
	Drop();
	name_ = cadr(s);

	Object *rules = caddr(s);
	Object *ids = cadr(rules);
	while (ids != Null) {
		if (!car(ids)->IsSymbol()) // Error
			return nullptr;
		identifier_.insert(car(ids)->Symbol());
		ids = cdr(ids);
	}
	Object *entry = cddr(rules);
	while (entry != Null) {
		Object *pair = car(entry);
		Object *pattern = car(pair);
		Object *tmpl = cadr(pair);
		bool ok = Match(pattern, o);
		if (ok) {
			return DoExtend(tmpl);
		}
		Drop();
		entry = cdr(entry);
	}
	return nullptr;
}

Object *MacroAnalyzer::DoExtend(Object *t) {
	if (!t->IsPair()) { // If not list
		if (!t->IsSymbol())
			return t;
		Object *o = Binded(t->Symbol());
		return !o ? t : o;
	}

	// Templete is a list:
	std::vector<Object*> tl(std::move(List2Vector(t)));
	std::vector<Object*> ol;
	Object *prev = nullptr;
	for (auto elem : tl) {
		switch (elem->OwnedType()) {
		case values::SYMBOL:
			if (elem == Kof(EllipsisSymbol)) {
				DCHECK(prev);
				DCHECK(prev->IsSymbol());

				auto binds = Ellipsis(prev->Symbol());
				for (auto binded : binds)
					ol.push_back(binded);
			} else {
				Object *binded = Binded(elem->Symbol());
				ol.push_back(!binded ? elem : binded);
			}
			break;
		case values::PAIR:
			ol.push_back(DoExtend(elem));
			break;
		default:
			ol.push_back(elem);
			break;
		}
		prev = elem;
	}
	return Vector2List(ol);
}

bool MacroAnalyzer::Match(Object *pattern, Object *o) {
	std::vector<Object*> pl(std::move(List2Vector(pattern))),
		ol(std::move(List2Vector(o)));
	if (ol.size() == 0 && pl.size() > 0)
		return false;
	size_t j = 0;
	for (size_t i = 0; i < pl.size(); ++i) {
		bool ok = false;
		if (pl[i] == Kof(UnderLineSymbol) &&
				ol[j] == name_) {
			ok = true;
		} else if (pl[i] == Kof(EllipsisSymbol)) {
			if (!i)
				return false; // Error;
			size_t remain = pl.size() - i - 1;
			if (ol.size() >= j + remain) {
			// a0 a1 a2 ... a4 a5 i = 3 remian = 6 - 3 - 1
			// b0 b1 b2 b3 b4 b5
				const int end = ol.size() - remain;
				for (int k = j; k < end; ++k) {
					if (!BindEllipsis(pl[i - 1], ol[k])) {
						return false;
					}
				}
				j = end - 1;
				ok = true;
			}
		} else if (pl[i]->IsSymbol()) {
			if (Identifier(pl[i]->Symbol())) {
				if (ol[j]->IsSymbol()) {
					ok = strcmp(pl[i]->Symbol(), ol[j]->Symbol()) == 0;
				}
			} else {
				binded_[pl[i]->Symbol()] = ol[j];
				ok = true;
			}
		} else if (pl[i]->IsPair()) {
			if (ol[j]->IsPair())
				ok = Match(pl[i], ol[j]);
		} else if (pl[i]->OwnedType() == ol[j]->OwnedType()) {
			ok = true;
		}
		if (!ok)
			return false;
		if (++j >= ol.size())
			break;
	}
	return j == ol.size();
}

bool MacroAnalyzer::BindEllipsis(Object *p, Object *o) {
	if (p->IsSymbol()) {
		MutableEllipsis(p->Symbol())->push_back(o);
		return true;
	}
	if (!p->IsPair() || !o->IsPair())
		return false;
	std::vector<Object*> pl(std::move(List2Vector(p))),
		                 ol(std::move(List2Vector(o)));
	if (pl.size() != ol.size())
		return false;
	for (size_t i = 0; i < pl.size(); ++i) {
		if (!BindEllipsis(pl[i], ol[i]))
			return false;
	}
	return true;
}

std::vector<Object*> *MacroAnalyzer::MutableEllipsis(const std::string &name) {
	auto iter = ellipsis_.find(name);
	if (iter != ellipsis_.end())
		return iter->second;
	std::vector<Object*> *vec = new std::vector<Object*>();
	ellipsis_[name] = vec;
	return vec;
}

std::vector<Object*> MacroAnalyzer::List2Vector(Object *list) {
	std::vector<Object*> rv;
	while (list != Null) {
		rv.push_back(car(list));
		list = cdr(list);
	}
	return std::move(rv);
}

Object *MacroAnalyzer::Vector2List(const std::vector<Object*> &flat) {
	Object *o, *p = Null;
	auto i = flat.size();
	while (i--) {
		o = obm_->Cons(flat[i], p);
		p = o;
	}
	return o;
}

void MacroAnalyzer::Drop() {
	for (auto pair : ellipsis_)
		delete pair.second;
	ellipsis_.clear();
	binded_.clear();
	identifier_.clear();
}

#undef Null
#undef Kof
} // namespace vm
} // namespace ajimu

