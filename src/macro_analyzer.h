#ifndef AJIMU_VM_MACRO_ANALYZER_H
#define AJIMU_VM_MACRO_ANALYZER_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace ajimu {
namespace values {
class ObjectManagement;
class Object;
} // namespace values
namespace vm {

class MacroAnalyzer {
public:
	MacroAnalyzer(values::ObjectManagement *obm)
		: obm_(obm) {
	}

	~MacroAnalyzer() {
		Drop();
	}

	values::Object *Extend(values::Object *s, values::Object *o);

private:
	MacroAnalyzer(const MacroAnalyzer &) = delete;
	void operator = (const MacroAnalyzer &) = delete;

	values::Object *DoExtend(values::Object *t);

	bool Match(values::Object *pattern, values::Object *o);

	bool BindEllipsis(values::Object *p, values::Object *o);

	std::vector<values::Object*> *MutableEllipsis(const std::string &name);

	const std::vector<values::Object*> &
		Ellipsis(const std::string &name) const {
		auto iter = ellipsis_.find(name);
		return iter == ellipsis_.end() ? kEmptyVector : *iter->second;
	}

	values::Object *Binded(const std::string &name) const {
		auto iter = binded_.find(name);
		return iter == binded_.end() ? nullptr : iter->second;
	}

	bool Identifier(const std::string &name) const {
		return identifier_.find(name) != identifier_.end();
	}

	std::vector<values::Object*> List2Vector(values::Object *list);

	values::Object *Vector2List(const std::vector<values::Object*> &flat);

	void Drop();

	values::ObjectManagement *obm_;
	const values::Object *name_;
	std::unordered_set<std::string> identifier_;
	std::unordered_map<std::string, values::Object*> binded_;
	std::unordered_map<std::string, std::vector<values::Object*>*> ellipsis_;
	const std::vector<values::Object*> kEmptyVector;
}; // class MacroAnalyzer

} // namespace vm
} // namespace ajimu

#endif //AJIMU_VM_MACRO_ANALYZER_H

