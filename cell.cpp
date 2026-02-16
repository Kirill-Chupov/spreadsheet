#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <sstream>
#include <algorithm>


class Cell::Impl {
public:
	virtual Cell::Value GetValue() const = 0;
	virtual std::string GetText() const = 0;

	virtual void InvalidateCache() const {
		return;
	}

	virtual std::vector<Position> GetReferencedCells() const {
		return {};
	}

	virtual ~Impl() = default;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
	Cell::Value GetValue() const override {
		return static_cast<double>(0);
	}

	std::string GetText() const override {
		return std::string{};
	}
};

class Cell::TextImpl : public Cell::Impl {
public:
	explicit TextImpl(std::string text)
		: text_{ std::move(text) } {

	}

	Cell::Value GetValue() const override {
		if (!text_.empty() && text_[0] == ESCAPE_SIGN) {
			return text_.substr(1);
		}
		return text_;
	}

	std::string GetText() const override {
		return text_;
	}

private:
	std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
	explicit FormulaImpl(std::string text, Sheet& sheet)
		: formula_{ ParseFormula(std::move(text)) }
		, sheet_{ sheet } {

	}

	Cell::Value GetValue() const override {
		if (cache_.has_value()) {
			return cache_.value();
		}

		auto result = formula_->Evaluate(sheet_);
		if (std::holds_alternative<double>(result)) {
			cache_ = std::get<double>(result);
			return cache_.value();
		}

		if (std::holds_alternative<FormulaError>(result)) {
			cache_ = std::get<FormulaError>(result);
			return cache_.value();
		}

		return std::string{ "Not value" };
	}

	std::string GetText() const override {
		return "=" + formula_->GetExpression();
	}

	void InvalidateCache() const override {
		cache_.reset();
	}

	std::vector<Position> GetReferencedCells() const override {
		return formula_->GetReferencedCells();
	}

private:
	std::unique_ptr<FormulaInterface> formula_;
	Sheet& sheet_;

	mutable std::optional<Cell::Value> cache_;
};


Cell::Cell(Sheet& sheet, Position pos)
	: impl_{ std::make_unique<EmptyImpl>() }
	, sheet_{ sheet }
	, pos_{ pos } {
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
	if (impl_ != nullptr && text == GetText()) {
		return;
	}

	//Хранит старые прямые зависимости
	auto old_referenced_cells = GetReferencedCells();

	if (text.empty()) {
		impl_ = std::make_unique<EmptyImpl>();
	} else if (text[0] == FORMULA_SIGN && text.size() > 1) {
		SetFormulaImpl(text.substr(1));
	} else {
		impl_ = std::make_unique<TextImpl>(std::move(text));
	}

	RemoveReverseRef(old_referenced_cells);
	AddReverseRef();
	InvalidateCache();
}

void Cell::Clear() {
	Set(std::string());
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
	return !reverse_ref_.empty();
}

void Cell::InvalidateCache() const {
	//Хранит обработаные ячейки
	std::unordered_set<const Cell*> proccesed_cells;

	InvalidateCache(proccesed_cells);
}

void Cell::InvalidateCache(std::unordered_set<const Cell*>& proccesed_cells) const {
	impl_->InvalidateCache();
	proccesed_cells.insert(this);

	for (const Cell* cell : reverse_ref_) {
		if (proccesed_cells.find(cell) != proccesed_cells.end()) {
			continue;
		}

		if (cell != nullptr) {
			cell->InvalidateCache(proccesed_cells);
		}
	}
}

void Cell::AddReverseRef() {
	for (auto pos : GetReferencedCells()) {
		if (sheet_.GetCell(pos) == nullptr) {
			sheet_.SetCell(pos, std::string());
		}

		if (Cell* cell_ptr = dynamic_cast<Cell*>(sheet_.GetCell(pos))) {
			cell_ptr->reverse_ref_.insert(this);
		}
	}
}

void Cell::RemoveReverseRef(const std::vector<Position>& old_referenced_cells) {
	for (auto pos : old_referenced_cells) {
		if (Cell* cell_ptr = dynamic_cast<Cell*>(sheet_.GetCell(pos))) {
			cell_ptr->reverse_ref_.erase(this);
		}
	}
}

void Cell::FindCircularDependency() const {
	//Отслеживает обход графа
	std::unordered_map<Position, Color, PositionHasher> marker;
	//vector используется в качестве стека, позволяет восстановить цикл
	std::vector<Position> trace;

	try {
		FindCircularDependency(marker, trace, pos_);
	} catch (const CircularDependencyException& ex) {
		throw CircularDependencyException("Find cycle: " + StackToString(trace));
	}
}

void Cell::FindCircularDependency(std::unordered_map<Position, Cell::Color, PositionHasher>& marker, std::vector<Position>& trace, Position cur_pos) const {
	trace.push_back(cur_pos);
	auto it = marker.find(cur_pos);

	if (it != marker.end() && it->second == Color::GRAY) {
		throw CircularDependencyException("");
	}

	marker[cur_pos] = Color::GRAY;

	if (const auto* cell = sheet_.GetCell(cur_pos)) {
		const auto& vec = cell->GetReferencedCells();

		for (auto pos : vec) {
			FindCircularDependency(marker, trace, pos);
		}
	}

	marker[cur_pos] = Color::BLACK;
	trace.pop_back();
}

std::string Cell::StackToString(const std::vector<Position>& trace) const {
	if (trace.empty()) {
		return {};
	}

	std::ostringstream oss;

	//Ищем начало цикла
	const auto begin_cycle = std::find(trace.begin(), trace.end(), trace.back());

	for (auto it = begin_cycle; it != trace.end(); ++it) {
		if (it != begin_cycle) {
			oss << "->";
		}
		oss << it->ToString();
	}

	return oss.str();
}

void Cell::SetFormulaImpl(std::string text) {
	std::unique_ptr<Impl> old_impl = std::move(impl_);

	try {
		impl_ = std::make_unique<FormulaImpl>(std::move(text), sheet_);
		FindCircularDependency();
	} catch (...) {
		impl_ = std::move(old_impl);
		throw;
	}
}