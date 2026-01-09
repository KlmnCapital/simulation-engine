export module simulation_engine:template_instantiations;

import :market_data;
import :engine;
import :probability_distributions;
import :statistics;
import :market_state;
import :market_data;
import :portfolio;

export namespace sim {

// Note: Templates with implementations in .cpp files (Portfolio, Engine, Statistics, MarketDataParquet)
// are explicitly instantiated in their respective .cpp files to avoid duplicate instantiation errors.

// RunParams struct instantiations (header-only)
template struct RunParams<ConstantDistribution>;

// RunningStatistics class instantiations (header-only)
template class RunningStatistics<ConstantDistribution>;

// IStrategy class instantiations (header-only)
template class IStrategy<10, 1, ConstantDistribution>;
template class IStrategy<10, 4, ConstantDistribution>;

// IMarketData class instantiations (header-only)
template class IMarketData<10, 4>;
template class IMarketData<10, 1>;

// MarketState struct instantiations (header-only)
template struct MarketState<10, 4>;
template struct MarketState<10, 1>;

}  // namespace sim
