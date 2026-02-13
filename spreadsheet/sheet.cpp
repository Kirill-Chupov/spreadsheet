#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {
}

void Sheet::SetCell(Position pos, std::string text) {
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid position");
	}

	if (sheet_.find(pos) == sheet_.end()) {
		sheet_[pos] = std::make_unique<Cell>(*this, pos);
		UpdateSizeSheet(pos, 1);
	}

	//Метод доступен только у Cell
	if (Cell* cell_ptr = dynamic_cast<Cell*>(sheet_[pos].get())) {
		cell_ptr->Set(std::move(text));
	}
}

const CellInterface* Sheet::GetCell(Position pos) const {
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid position");
	}

	return sheet_.find(pos) != sheet_.end() ? sheet_.at(pos).get() : nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid position");
	}

	return sheet_.find(pos) != sheet_.end() ? sheet_.at(pos).get() : nullptr;
}

void Sheet::ClearCell(Position pos) {
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid position");
	}

	if (sheet_.find(pos) == sheet_.end()) {
		return;
	}

	Cell* cell_ptr = dynamic_cast<Cell*>(sheet_[pos].get());
	if (cell_ptr == nullptr) {
		return;
	}

	cell_ptr->Clear();

	if (!cell_ptr->IsReferenced()) {
		sheet_.erase(pos);

		UpdateSizeSheet(pos, -1);
	}
}

Size Sheet::GetPrintableSize() const {
	int rows = static_cast<int>(counter_in_row_.size());
	int cols = static_cast<int>(counter_in_col_.size());
	return Size{rows, cols};
}

void Sheet::PrintValues(std::ostream& output) const {
	const Size size = GetPrintableSize();
	Position pos;
	for (pos.row = 0; pos.row < size.rows; ++pos.row) {
		for (pos.col = 0; pos.col < size.cols; ++pos.col) {
			auto cell_ptr = GetCell(pos);
			output << (pos.col == 0 ? "" : "\t");

			if (cell_ptr == nullptr) {
				output << "";
				continue;
			}

			std::visit([&](const auto& x) { output << x; }, cell_ptr->GetValue());
		}
		output << '\n';
	}
}

void Sheet::PrintTexts(std::ostream& output) const {
	const Size size = GetPrintableSize();
	Position pos;
	for (pos.row = 0; pos.row < size.rows; ++pos.row) {
		for (pos.col = 0; pos.col < size.cols; ++pos.col) {
			auto cell_ptr = GetCell(pos);
			output << (pos.col == 0 ? "" : "\t");

			if (cell_ptr == nullptr) {
				output << "";
				continue;
			}

			output << cell_ptr->GetText();
		}
		output << '\n';
	}
}

void Sheet::UpdateSizeSheet(const Position& pos, int delta) {
	if (pos.row >= static_cast<int>(counter_in_row_.size())) {
		counter_in_row_.resize(pos.row + 1);
	}

	if (pos.col >= static_cast<int>(counter_in_col_.size())) {
		counter_in_col_.resize(pos.col + 1);
	}

	counter_in_row_[pos.row] += delta;
	counter_in_col_[pos.col] += delta;

	auto predicate = [](int x) {return x > 0; };

	if (delta < 0) {
		auto it_row = std::find_if(counter_in_row_.rbegin(), counter_in_row_.rend(), predicate);
		auto it_col = std::find_if(counter_in_col_.rbegin(), counter_in_col_.rend(), predicate);

		auto new_size_row = std::distance(it_row, counter_in_row_.rend());
		auto new_size_col = std::distance(it_col, counter_in_col_.rend());

		counter_in_row_.resize(new_size_row);
		counter_in_col_.resize(new_size_col);
	}
}

std::unique_ptr<SheetInterface> CreateSheet() {
	return std::make_unique<Sheet>();
}