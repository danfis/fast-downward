#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_STATELESS_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_STATELESS_H

#include "merge_strategy_factory.h"

namespace options {
class Options;
}

namespace merge_and_shrink {
class MergeSelector;
class MergeStrategyFactoryStateless : public MergeStrategyFactory {
    std::shared_ptr<MergeSelector> merge_selector;
protected:
    virtual std::string name() const override;
    virtual void dump_strategy_specific_options() const override;
public:
    explicit MergeStrategyFactoryStateless(options::Options &options);
    virtual ~MergeStrategyFactoryStateless() override = default;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const std::shared_ptr<AbstractTask> &task,
        FactoredTransitionSystem &fts) override;
};
}

#endif
