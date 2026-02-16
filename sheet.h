#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::unordered_map<Position, std::unique_ptr<CellInterface>, PositionHasher> sheet_;
    //Счётчики для определения Size
    std::vector<int> counter_in_row_;
    std::vector<int> counter_in_col_;

private:
    void UpdateSizeSheet(const Position& pos, int delta);
};