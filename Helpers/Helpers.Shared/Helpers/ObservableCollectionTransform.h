#pragma once
#include "common.h"
#include "IObservableCollection.h"
#include "Event.hpp"

#include <memory>
#include <algorithm>
#include <iterator>

namespace HELPERS_NS {
    template<typename DstT, typename SrcT, typename TransformFn>
    class ObservableCollectionTransform final : public IObservableCollection<DstT> {
    public:
        ObservableCollectionTransform(
            TransformFn transformFn,
            IObservableCollection<SrcT>& src,
            std::shared_ptr<void> srcHolder)
            : srcHolder(std::move(srcHolder))
            , src(src)
            , transformFn(std::move(transformFn))
        {
            auto& srcEvt = this->src.GetOnChangedEvent();

            this->onChangedSrcToken = srcEvt.Add(
                [this](const ObservableCollectionChangedArgs<SrcT>& srcArgs)
                {
                    ObservableCollectionChangedArgs<DstT> dstArgs;

                    dstArgs.action = srcArgs.action;

                    dstArgs.newItems.reserve(srcArgs.newItems.size());
                    std::transform(
                        srcArgs.newItems.begin(),
                        srcArgs.newItems.end(),
                        std::back_inserter(dstArgs.newItems),
                        this->transformFn);

                    dstArgs.newStartingIndex = srcArgs.newStartingIndex;

                    dstArgs.oldItems.reserve(srcArgs.oldItems.size());
                    std::transform(
                        srcArgs.oldItems.begin(),
                        srcArgs.oldItems.end(),
                        std::back_inserter(dstArgs.oldItems),
                        this->transformFn);

                    dstArgs.oldStartingIndex = srcArgs.oldStartingIndex;

                    this->onChangedEvent(dstArgs);
                });
        }

        IEvent<const ObservableCollectionChangedArgs<DstT>&>& GetOnChangedEvent() const override {
            return this->onChangedEvent;
        }

        size_t GetCount() override {
            return this->src.GetCount();
        }

        DstT GetAt(size_t idx) override {
            return this->transformFn(this->src.GetAt(idx));
        }

        void Add(DstT item) override {
            this->src.Add(this->transformFn(item));
        }

        void Remove(DstT item) override {
            this->src.Remove(this->transformFn(item));
        }

    private:
        mutable Event<const ObservableCollectionChangedArgs<DstT>&> onChangedEvent;
        std::shared_ptr<void> srcHolder;
        IObservableCollection<SrcT>& src;
        TransformFn transformFn;

        std::shared_ptr<void> onChangedSrcToken;
    };

    template<typename SrcDstTransformFn, typename DstSrcTransformFn>
    std::shared_ptr < IObservableCollection<decltype(SrcDstTransformFn()({})) >> MakeObservableCollectionTransform(
        SrcDstTransformFn srcDstTransformFn,
        DstSrcTransformFn dstSrcTransformFn,
        IObservableCollection<decltype(DstSrcTransformFn()({})) > & src,
        std::shared_ptr<void> srcHolder)
    {
        using SrcT = decltype(DstSrcTransformFn()({}));
        using DstT = decltype(SrcDstTransformFn()({}));

        struct Transform {
            Transform(
                SrcDstTransformFn srcDstTransformFn,
                DstSrcTransformFn dstSrcTransformFn)
                : srcDstTransformFn(std::move(srcDstTransformFn))
                , dstSrcTransformFn(std::move(dstSrcTransformFn))
            {}

            DstT operator()(const SrcT& src) {
                return this->srcDstTransformFn(src);
            }

            SrcT operator()(const DstT& dst) {
                return this->dstSrcTransformFn(dst);
            }

            SrcDstTransformFn srcDstTransformFn;
            DstSrcTransformFn dstSrcTransformFn;
        };

        return std::make_shared<ObservableCollectionTransform<DstT, SrcT, Transform>>(
            Transform(
                std::move(srcDstTransformFn),
                std::move(dstSrcTransformFn)),
            src,
            std::move(srcHolder)
        );
    }

    template<typename SrcDstTransformFn, typename DstSrcTransformFn>
    std::shared_ptr < IObservableCollection<decltype(SrcDstTransformFn()({})) >> MakeObservableCollectionTransform(
        SrcDstTransformFn srcDstTransformFn,
        DstSrcTransformFn dstSrcTransformFn,
        std::shared_ptr < IObservableCollection<decltype(DstSrcTransformFn()({})) >> src)
    {
        return MakeObservableCollectionTransform(std::move(srcDstTransformFn), std::move(dstSrcTransformFn), *src, src);
    }
}
