/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace venus {

/**
 * @brief A move-only `void()` callable, like std::function<void()> but without the requirement to be copyable.
 *
 * This allows tasks that own move-only state, like a std::packaged_task or a std::unique_ptr,
 * without wrapping them in a std::shared_ptr.
 */
class task
{
public:
    task() = default;

    template <typename Fn, typename = typename std::enable_if<!std::is_same<typename std::decay<Fn>::type, task>::value>::type>
    task(Fn && fn) : // NOLINT(google-explicit-constructor): implicit, like std::function
        m_callable(std::make_unique<model<typename std::decay<Fn>::type>>(std::forward<Fn>(fn)))
    {
    }

    task(task &&) noexcept = default;
    task & operator=(task &&) noexcept = default;
    task(const task &) = delete;
    task & operator=(const task &) = delete;
    ~task() = default;

    void operator()()
    {
        m_callable->call();
    }

    [[nodiscard]] explicit operator bool() const
    {
        return m_callable != nullptr;
    }

private:
    struct concept_t
    {
        concept_t() = default;
        concept_t(const concept_t &) = delete;
        concept_t & operator=(const concept_t &) = delete;
        concept_t(concept_t &&) = delete;
        concept_t & operator=(concept_t &&) = delete;
        virtual ~concept_t() = default;
        virtual void call() = 0;
    };

    template <typename Fn>
    struct model final : concept_t
    {
        explicit model(Fn && fn) :
            m_fn(std::move(fn))
        {
        }

        explicit model(const Fn & fn) :
            m_fn(fn)
        {
        }

        void call() override
        {
            m_fn();
        }

        Fn m_fn;
    };

    std::unique_ptr<concept_t> m_callable;
};

} // namespace venus
