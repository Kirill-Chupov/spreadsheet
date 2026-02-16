#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <unordered_set>
#include <unordered_map>
#include <vector>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet, Position pos);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;
    bool IsReferenced() const;


private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;

    Sheet& sheet_;
    const Position pos_;

    //Контейнер ячеек которые зависят от текущей
    std::unordered_set<const Cell*> reverse_ref_;

private:
    void InvalidateCache() const;
    void FindCircularDependency() const;

    void AddReverseRef();
    void RemoveReverseRef(const std::vector<Position>& old_referenced_cells);

    //Покраска ячейки при обходе графа
    enum class Color {
        WHITE,
        GRAY,
        BLACK
    };

    //Вспомогательные методы для рекурсивного обхода
    void InvalidateCache(std::unordered_set<const Cell*>& proccesed_cells) const;
    void FindCircularDependency(std::unordered_map<Position, Cell::Color, PositionHasher>& marker, std::vector<Position>& trace, Position cur_pos) const;

    //Извлекает цикл из стека
    std::string StackToString(const std::vector<Position>& trace) const;

    //Создание формулы имеет сложную логику
    void SetFormulaImpl(std::string text);
};