#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <iostream>
#include <exception>

using namespace std::literals;

namespace {
	class Formula : public FormulaInterface {
	public:
		explicit Formula(std::string expression) try
			: ast_{ ParseFormulaAST(expression) }
		{

		} catch (const std::exception& ex) {
			throw FormulaException(ex.what());
		}

		Value Evaluate(const SheetInterface& sheet) const override {
			try {
				return ast_.Execute(sheet);
			} catch (const FormulaError& fe) {
				return fe;
			}
		}

		std::string GetExpression() const override {
			std::ostringstream os;
			ast_.PrintFormula(os);
			return os.str();
		}

		std::vector<Position> GetReferencedCells() const override {
			auto cells = ast_.GetCells();
			std::vector<Position> result{ cells.begin(), cells.end() };

			std::sort(result.begin(), result.end());
			auto it_del = std::unique(result.begin(), result.end());
			result.erase(it_del, result.end());

			return result;
		}

	private:
		FormulaAST ast_;
	};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
	return std::make_unique<Formula>(std::move(expression));
}