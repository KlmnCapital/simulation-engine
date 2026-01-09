// probability_distributions.cppm
export module simulation_engine:probability_distributions;

import std;

export namespace sim {

struct ConstantDistribution {
    double rate;
    double operator()(std::mt19937&) const { return rate; }
};

}  // namespace sim
