#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "orderbook.hpp"

namespace py = pybind11;

PYBIND11_MODULE(pyorderbook, m) {
    m.doc() = "High-frequency trading orderbook implementation";
    
    py::enum_<hft::Side>(m, "Side")
        .value("BUY", hft::Side::BUY)
        .value("SELL", hft::Side::SELL)
        .export_values();
    
    py::enum_<hft::OrderType>(m, "OrderType")
        .value("LIMIT", hft::OrderType::LIMIT)
        .value("MARKET", hft::OrderType::MARKET)
        .export_values();
    
    py::class_<hft::Order>(m, "Order")
        .def_readonly("order_id", &hft::Order::order_id)
        .def_readonly("side", &hft::Order::side)
        .def_readonly("price", &hft::Order::price)
        .def_readonly("quantity", &hft::Order::quantity)
        .def_readonly("timestamp", &hft::Order::timestamp);
    
    py::class_<hft::Trade>(m, "Trade")
        .def_readonly("buy_order_id", &hft::Trade::buy_order_id)
        .def_readonly("sell_order_id", &hft::Trade::sell_order_id)
        .def_readonly("price", &hft::Trade::price)
        .def_readonly("quantity", &hft::Trade::quantity)
        .def_readonly("timestamp", &hft::Trade::timestamp)
        .def("__repr__", [](const hft::Trade& t) {
            return "<Trade buy=" + std::to_string(t.buy_order_id) + 
                   " sell=" + std::to_string(t.sell_order_id) +
                   " price=" + std::to_string(t.price) +
                   " qty=" + std::to_string(t.quantity) + ">";
        });
    
    py::class_<hft::OrderBook>(m, "OrderBook")
        .def(py::init<const std::string&>(), py::arg("symbol"))
        .def("add_limit_order", &hft::OrderBook::add_limit_order,
             py::arg("order_id"), py::arg("side"), py::arg("price"), py::arg("quantity"),
             "Add a limit order to the orderbook")
        .def("add_market_order", &hft::OrderBook::add_market_order,
             py::arg("order_id"), py::arg("side"), py::arg("quantity"),
             "Add a market order to the orderbook")
        .def("cancel_order", &hft::OrderBook::cancel_order,
             py::arg("order_id"),
             "Cancel an existing order")
        .def("modify_order", &hft::OrderBook::modify_order,
             py::arg("order_id"), py::arg("new_quantity"),
             "Modify the quantity of an existing order")
        .def("get_best_bid", &hft::OrderBook::get_best_bid,
             "Get the best bid price")
        .def("get_best_ask", &hft::OrderBook::get_best_ask,
             "Get the best ask price")
        .def("get_mid_price", &hft::OrderBook::get_mid_price,
             "Get the mid price")
        .def("get_spread", &hft::OrderBook::get_spread,
             "Get the bid-ask spread")
        .def("get_bid_volume_at_price", &hft::OrderBook::get_bid_volume_at_price,
             py::arg("price"),
             "Get total bid volume at a specific price")
        .def("get_ask_volume_at_price", &hft::OrderBook::get_ask_volume_at_price,
             py::arg("price"),
             "Get total ask volume at a specific price")
        .def("get_total_bid_volume", &hft::OrderBook::get_total_bid_volume,
             "Get total volume on bid side")
        .def("get_total_ask_volume", &hft::OrderBook::get_total_ask_volume,
             "Get total volume on ask side")
        .def("get_bids", &hft::OrderBook::get_bids,
             py::arg("depth") = 10,
             "Get top N bid levels")
        .def("get_asks", &hft::OrderBook::get_asks,
             py::arg("depth") = 10,
             "Get top N ask levels")
        .def("get_trades", &hft::OrderBook::get_trades,
             "Get all executed trades")
        .def("get_order_count", &hft::OrderBook::get_order_count,
             "Get current number of orders in the book")
        .def("get_trade_count", &hft::OrderBook::get_trade_count,
             "Get total number of executed trades")
        .def("get_symbol", &hft::OrderBook::get_symbol,
             "Get the symbol for this orderbook")
        .def("__repr__", [](const hft::OrderBook& ob) {
            std::string result = "<OrderBook symbol=" + ob.get_symbol();
            auto bid = ob.get_best_bid();
            auto ask = ob.get_best_ask();
            if (bid && ask) {
                result += " bid=" + std::to_string(*bid) + 
                         " ask=" + std::to_string(*ask);
            }
            result += " orders=" + std::to_string(ob.get_order_count()) + ">";
            return result;
        });
}
