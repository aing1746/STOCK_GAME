#include <cmath>
#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <vector>
#include <limits>
#include <print>
#include <format>



struct Stock {
    std::string name;
    int base_price;    // Initial reference price
    int current_price; // Accumulated from previous close

    Stock(const std::string& n, int p)
        : name(n)
        , base_price(p)
        , current_price(p) {}
};

struct OwnedStock {
    std::string name;
    int quantity;

    OwnedStock(const std::string& n, int q)
        : name(n)
        , quantity(q) {}
};


class GameManager {
public:
    long long cash = 1'000'000;
    int day = 0;
    std::vector<Stock> stock_list;
    std::vector<OwnedStock> stock_have;

    // -- Search ---------------------------------------------------------------
    Stock* find_stock(const std::string& name) {
        auto it = std::find_if(
            stock_list.begin(), stock_list.end(),
            [&](const Stock& s) { return s.name == name; }
        );
        return it != stock_list.end() ? &(*it) : nullptr;
    }

    OwnedStock* find_owned(const std::string& name) {
        auto it = std::find_if(
            stock_have.begin(), stock_have.end(),
            [&](const OwnedStock& o) { return o.name == name; }
        );
        return it != stock_have.end() ? &(*it) : nullptr;
    }

    // -- Price update (accumulated from previous close, floor at 1) -----------
    void update_prices() {
        for (auto& s : stock_list) {
            int delta = static_cast<int>(s.current_price * (random_fluctuation() / 10.0));
            s.current_price = std::max(1, s.current_price + delta);
        }
    }

    // -- Buy ------------------------------------------------------------------
    enum class BuyResult { OK, NOT_FOUND, BAD_QTY, NO_FUNDS };

    BuyResult buy(const std::string& name, int qty) {
        if (qty <= 0) return BuyResult::BAD_QTY;
        Stock* s = find_stock(name);
        if (!s) return BuyResult::NOT_FOUND;

        long long cost = static_cast<long long>(s->current_price) * qty;
        if (cost > cash) return BuyResult::NO_FUNDS;

        cash -= cost;
        OwnedStock* owned = find_owned(name);
        if (owned) owned->quantity += qty;
        else stock_have.emplace_back(name, qty);
        return BuyResult::OK;
    }

    // -- Sell -----------------------------------------------------------------
    enum class SellResult { OK, NOT_OWNED, BAD_QTY, EXCEEDS_QTY };

    SellResult sell(const std::string& name, int qty) {
        if (qty <= 0) return SellResult::BAD_QTY;
        OwnedStock* owned = find_owned(name);
        Stock* s = find_stock(name);
        if (!owned || !s) return SellResult::NOT_OWNED;
        if (qty > owned->quantity) return SellResult::EXCEEDS_QTY;

        cash += static_cast<long long>(s->current_price) * qty;
        owned->quantity -= qty;
        if (owned->quantity == 0)
            stock_have.erase(
                std::remove_if(
                    stock_have.begin(), stock_have.end(),
                    [&](const OwnedStock& o) { return o.name == name; }
                ),
                stock_have.end()
            );
        return SellResult::OK;
    }

    // -- Stock list edit ------------------------------------------------------
    void add_or_update_stock(const std::string& name, int price) {
        Stock* existing = find_stock(name);
        if (existing) {
            existing->base_price = price;
            existing->current_price = price;
        } else {
            stock_list.emplace_back(name, price);
        }
    }

    bool delete_stock(const std::string& name) {
        auto before = stock_list.size();
        stock_list.erase(
            std::remove_if(
                stock_list.begin(), stock_list.end(),
                [&](const Stock& s) { return s.name == name; }
            ),
            stock_list.end()
        );
        return stock_list.size() < before;
    }

    // -- Totals ---------------------------------------------------------------
    long long total_valuation() const {
        long long total = cash;
        for (const auto& o : stock_have) {
            auto it = std::find_if(
                stock_list.begin(), stock_list.end(),
                [&](const Stock& s) { return s.name == o.name; }
            );
            if (it != stock_list.end())
                total += static_cast<long long>(it->current_price) * o.quantity;
        }
        return total;
    }

    // -- Reset ----------------------------------------------------------------
    void reset() {
        day = 0;
        cash = 1'000'000;
        stock_have.clear();
        for (auto& s : stock_list) s.current_price = s.base_price;
    }

private:
    std::mt19937 gen{std::random_device{}()};

    double random_fluctuation() {
        std::uniform_real_distribution<> dist(-2.0, 2.0);
        return std::round(dist(gen) * 10.0) / 10.0;
    }
};

class UI {
public:
    explicit UI(GameManager& gm) : gm(gm) {}

    void run() { menu(); }

private:
    GameManager& gm;

    // -- Input helpers --------------------------------------------------------
    void clear_screen() const {
        for (int i = 0; i < 40; i++) std::println("");
    }

    int input_int(const std::string& prompt) {
        int val;
        while (true) {
            std::print("{}", prompt);
            if (std::cin >> val) {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return val;
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::println("  [Error] Please enter a number.");
        }
    }

    std::string input_str(const std::string& prompt) {
        std::string val;
        std::print("{}", prompt);
        std::cin >> val;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return val;
    }

    // -- Display helpers ------------------------------------------------------
    void print_stock_table() const {
        std::println("{:<12} {:>10} {:>10} {:>14}", "Stock", "Price", "Qty", "Valuation");
        std::println("{:-<50}", "");
        for (const auto& s : gm.stock_list) {
            auto it = std::find_if(
                gm.stock_have.begin(), gm.stock_have.end(),
                [&](const OwnedStock& o) { return o.name == s.name; }
            );
            if (it != gm.stock_have.end()) {
                long long val = static_cast<long long>(s.current_price) * it->quantity;
                std::println(
                    "{:<12} {:>10} {:>9}sh {:>14}",
                    s.name, s.current_price, it->quantity, val
                );
            } else {
                std::println(
                    "{:<12} {:>10} {:>10} {:>14}",
                    s.name, s.current_price, "-", "-"
                );
            }
        }
        std::println("{:-<50}", "");
    }

    void print_market_news() const {
        if (gm.stock_list.empty()) {
            std::println(" No stocks listed yet.");
            return;
        }

        const Stock* top_gainer = nullptr;
        const Stock* top_loser = nullptr;
        double best_rate = 0.0;
        double worst_rate = 0.0;
        double avg_rate = 0.0;

        for (const auto& s : gm.stock_list) {
            if (s.base_price <= 0) continue;
            double rate = static_cast<double>(s.current_price - s.base_price)
                / s.base_price * 100.0;
            avg_rate += rate;
            if (rate > best_rate) {
                best_rate = rate;
                top_gainer = &s;
            }
            if (rate < worst_rate) {
                worst_rate = rate;
                top_loser = &s;
            }
        }
        avg_rate /= static_cast<double>(gm.stock_list.size());

        if (avg_rate > 3.0) std::println(" [MARKET] Strong rally -- broad gains across all sectors.");
        else if (avg_rate > 0.5) std::println(" [MARKET] Market up slightly -- cautious optimism.");
        else if (avg_rate > -0.5) std::println(" [MARKET] Market flat -- investors in wait-and-see mode.");
        else if (avg_rate > -3.0) std::println(" [MARKET] Market slipping -- selling pressure building.");
        else std::println(" [MARKET] Heavy selloff -- panic spreading across sectors.");

        std::println(" -----------------------------------------------");

        if (top_gainer) {
            if (best_rate > 10.0) std::println(" > {} surges {:.1f}% -- analysts raise price targets.", top_gainer->name, best_rate);
            else if (best_rate > 5.0) std::println(" > {} climbs {:.1f}% on strong earnings outlook.", top_gainer->name, best_rate);
            else if (best_rate > 0.0) std::println(" > {} edges up {:.1f}% amid light buying interest.", top_gainer->name, best_rate);
        }
        if (top_loser) {
            if (worst_rate < -10.0) std::println(" > {} crashes {:.1f}% -- investors flee amid bad news.", top_loser->name, worst_rate);
            else if (worst_rate < -5.0) std::println(" > {} drops {:.1f}% as sell-off continues.", top_loser->name, worst_rate);
            else if (worst_rate < 0.0) std::println(" > {} dips {:.1f}% -- profit-taking seen.", top_loser->name, worst_rate);
        }
        if (!gm.stock_have.empty()) {
            const std::string& held = gm.stock_have.front().name;
            for (const auto& s : gm.stock_list) {
                if (s.name == held && s.base_price > 0) {
                    double hr = static_cast<double>(s.current_price - s.base_price)
                        / s.base_price * 100.0;
                    if (hr > 5.0) std::println(" > Your holding {} is up {:.1f}% -- consider taking profit.", s.name, hr);
                    else if (hr < -5.0) std::println(" > Your holding {} is down {:.1f}% -- watch for a recovery.", s.name, hr);
                    break;
                }
            }
        }
    }

    // -- Screens --------------------------------------------------------------
    void game_loop() {
        while (true) {
            clear_screen();
            std::println("=== STOCK GAME ===================================================");
            std::println(
                " DAY {:>3}  |  Cash: {:>14}  |  Total: {:>14}",
                gm.day, gm.cash, gm.total_valuation()
            );
            std::println("=================================================================");
            std::println();
            print_stock_table();
            std::println();
            std::println("  [B] Buy/Sell   [R] Market Info   [N] Next Day   [Q] Quit");
            std::println();

            std::string cmd = input_str("> ");
            if (cmd == "B" || cmd == "b") buy_screen();
            else if (cmd == "R" || cmd == "r") market_info_screen();
            else if (cmd == "N" || cmd == "n") {
                gm.day++;
                gm.update_prices();
            } else if (cmd == "Q" || cmd == "q") {
                std::println("  Exiting game.");
                break;
            }
        }
    }

    void buy_screen() {
        while (true) {
            clear_screen();
            std::println("=== BUY ==========================================================");
            print_stock_table();
            std::println("  Enter stock name then quantity  |  [@S] Sell  |  [@Q] Back");
            std::println();

            std::string name = input_str("> Stock name: ");
            if (name == "@Q") break;
            if (name == "@S") {
                sell_screen();
                continue;
            }

            int qty = input_int("> Quantity: ");

            switch (gm.buy(name, qty)) {
            case GameManager::BuyResult::OK:
                std::println("  [OK] Bought {} shares of {}. Cash: {}", qty, name, gm.cash);
                break;
            case GameManager::BuyResult::NOT_FOUND:
                std::println("  [Error] Stock not found.");
                break;
            case GameManager::BuyResult::BAD_QTY:
                std::println("  [Error] Quantity must be >= 1.");
                break;
            case GameManager::BuyResult::NO_FUNDS:
                std::println("  [Error] Insufficient funds.");
                break;
            }
        }
    }

    void sell_screen() {
        while (true) {
            clear_screen();
            std::println("=== SELL =========================================================");
            print_stock_table();
            std::println("  Enter stock name then quantity  |  [@B] Back");
            std::println();

            std::string name = input_str("> Stock name: ");
            if (name == "@B") break;

            int qty = input_int("> Quantity: ");

            switch (gm.sell(name, qty)) {
            case GameManager::SellResult::OK:
                std::println("  [OK] Sold {} shares of {}. Cash: {}", qty, name, gm.cash);
                break;
            case GameManager::SellResult::NOT_OWNED:
                std::println("  [Error] You do not own this stock.");
                break;
            case GameManager::SellResult::BAD_QTY:
                std::println("  [Error] Quantity must be >= 1.");
                break;
            case GameManager::SellResult::EXCEEDS_QTY:
                std::println("  [Error] Exceeds held quantity.");
                break;
            }
        }
    }

    void market_info_screen() {
        while (true) {
            clear_screen();
            std::println("=== MARKET INFO ==================================================");
            std::println();
            print_market_news();
            std::println();
            print_stock_table();
            std::println();
            std::println("  [@Q] Back");
            if (input_str("> ") == "@Q") break;
        }
    }

    void edit_screen() {
        auto print_list = [&]() {
            std::println("--- Registered Stocks ---");
            if (gm.stock_list.empty()) {
                std::println("  (none)");
            } else {
                for (const auto& s : gm.stock_list)
                    std::println("  {:<12} Base price: {:>8}", s.name, s.base_price);
            }
            std::println("-------------------------");
        };

        while (true) {
            clear_screen();
            std::println("=== STOCK EDITOR =================================================");
            print_list();
            std::println("  Enter stock name then price  |  [@D] Delete  |  [@Q] Back");
            std::println();

            std::string name = input_str("> Stock name: ");
            if (name == "@Q") break;

            if (name == "@D") {
                std::string del = input_str("> Stock to delete: ");
                if (gm.delete_stock(del)) std::println("  [OK] '{}' deleted.", del);
                else std::println("  [Error] '{}' not found.", del);
                continue;
            }

            int price = input_int("> Price: ");
            if (price <= 0) {
                std::println("  [Error] Price must be > 0.");
                continue;
            }

            gm.add_or_update_stock(name, price);
            std::println("  [OK] '{}' saved.", name);
            print_list();
        }
    }

    void menu() {
        while (true) {
            clear_screen();
            std::println("+------------------------------+");
            std::println("|       STOCK GAME v2.0        |");
            std::println("+------------------------------+");
            std::println("|  [S] Start Game              |");
            std::println("|  [E] Edit Stocks             |");
            std::println("|  [Q] Quit                    |");
            std::println("+------------------------------+");
            std::println();

            std::string cmd = input_str("> ");
            if (cmd == "S" || cmd == "s") {
                if (gm.stock_list.empty()) std::println("  [Info] Please add stocks first. [E]");
                else {
                    gm.reset();
                    game_loop();
                }
            } else if (cmd == "E" || cmd == "e") {
                edit_screen();
            } else if (cmd == "Q" || cmd == "q") {
                std::println("  Goodbye.");
                break;
            }
        }
    }
};


int main() {
    GameManager gm;
    UI ui(gm);
    ui.run();
    return 0;
}
