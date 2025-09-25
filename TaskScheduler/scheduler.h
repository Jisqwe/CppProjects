#include <iostream>
#include <memory>
#include <vector>
#include <utility>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <typeinfo>

struct Any {
    struct I {
        virtual ~I() {}
        virtual I* clone() const = 0;
    };

    template <class T>
    struct Holder : I {
        T value;
        explicit Holder(T v) : value(std::move(v)) {}
        I* clone() const override { return new Holder<T>(value); }
    };

    std::unique_ptr<I> ptr;

    Any() = default;

    template <class T>
    Any(T v) : ptr(new Holder<T>(std::move(v))) {}

    Any(const Any& o) : ptr(o.ptr ? o.ptr->clone() : nullptr) {}

    Any& operator=(const Any& o) {
        if (this != &o) {
            ptr.reset(o.ptr ? o.ptr->clone() : nullptr);
        }
        return *this;
    }

    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    template <class T>
    T& as() {
        auto* h = dynamic_cast<Holder<T>*>(ptr.get());
        if (!h) { throw std::bad_cast(); }
        return h->value;
    }

    template <class T>
    const T& as() const {
        auto const* h = dynamic_cast<const Holder<T>*>(ptr.get());
        if (!h) { throw std::bad_cast(); }
        return h->value;
    }

    bool has_value() const { return static_cast<bool>(ptr); }
};

struct ResultCellAny {
    mutable bool ready = false;
    mutable bool running = false;
    Any value;

    struct ITask {
        virtual ~ITask() = default;
        virtual void execute() = 0;
    };

    std::weak_ptr<ITask> producer;

    void ensureReady() const {
        if (ready) { return; }
        auto p = producer.lock();
        if (!p) { throw std::logic_error("no producer bound"); }
        if (running) { throw std::logic_error("cycle detected during ensureReady"); }
        running = true;
        p->execute();
        running = false;
        if (!ready) { throw std::runtime_error("producer executed but result not marked ready"); }
    }
};

template <class T>
class Future {
    std::shared_ptr<ResultCellAny> cell;

public:
    Future() = default;

    explicit Future(std::shared_ptr<ResultCellAny> c) : cell(std::move(c)) {}

    const T& get() const {
        cell->ensureReady();
        return cell->value.as<T>();
    }

    bool valid() const { return static_cast<bool>(cell); }

    std::shared_ptr<ResultCellAny> cellPtr() const { return cell; }
};

template <class X>
struct ArgWrap {
    X v;

    ArgWrap() = default;
    explicit ArgWrap(const X& x) : v(x) {}
    explicit ArgWrap(X&& x) : v(std::move(x)) {}

    const X& get() const { return v; }

    std::shared_ptr<ResultCellAny> depCell() const { return {}; }
};

template <class U>
struct ArgWrap<Future<U>> {
    Future<U> f;

    ArgWrap() = default;
    explicit ArgWrap(const Future<U>& fu) : f(fu) {}

    const U& get() const { return f.get(); }

    std::shared_ptr<ResultCellAny> depCell() const { return f.cellPtr(); }
};

struct TaskId {
    std::shared_ptr<ResultCellAny> out;

    TaskId() = default;
    explicit TaskId(std::shared_ptr<ResultCellAny> c) : out(std::move(c)) {}

    bool valid() const { return static_cast<bool>(out); }
};

class TTaskScheduler {
    struct ITaskBase {
        virtual ~ITaskBase() {}
        virtual void execute() = 0;
        virtual bool ready() const = 0;
        virtual std::shared_ptr<ResultCellAny> output() const = 0;
        virtual std::vector<std::shared_ptr<ResultCellAny>> deps() const = 0;
    };

    std::vector<std::shared_ptr<ITaskBase>> tasks_;

    static void bindProducer(const std::shared_ptr<ResultCellAny>& c,
                             const std::shared_ptr<ResultCellAny::ITask>& p) {
        c->producer = p;
    }

    template <class Callable>
    struct Task0 : ITaskBase, ResultCellAny::ITask {
        Callable fn;
        std::shared_ptr<ResultCellAny> out;

        Task0(Callable f, std::shared_ptr<ResultCellAny> o) : fn(std::move(f)), out(std::move(o)) {}

        void execute() override {
            if (out->ready) { return; }
            out->value = Any(fn());
            out->ready = true;
        }

        bool ready() const override { return out->ready; }
        std::shared_ptr<ResultCellAny> output() const override { return out; }
        std::vector<std::shared_ptr<ResultCellAny>> deps() const override { return {}; }
    };

    template <class Callable, class A1>
    struct Task1 : ITaskBase, ResultCellAny::ITask {
        Callable fn;
        ArgWrap<A1> a1;
        std::shared_ptr<ResultCellAny> out;

        Task1(Callable f, A1 x1, std::shared_ptr<ResultCellAny> o)
            : fn(std::move(f)), a1(std::move(x1)), out(std::move(o)) {}

        void execute() override {
            if (out->ready) { return; }
            out->value = Any(fn(a1.get()));
            out->ready = true;
        }

        bool ready() const override { return out->ready; }
        std::shared_ptr<ResultCellAny> output() const override { return out; }

        std::vector<std::shared_ptr<ResultCellAny>> deps() const override {
            std::vector<std::shared_ptr<ResultCellAny>> d;
            if (auto c = a1.depCell()) { d.push_back(std::move(c)); }
            return d;
        }
    };

    template <class Callable, class A1, class A2>
    struct Task2 : ITaskBase, ResultCellAny::ITask {
        Callable fn;
        ArgWrap<A1> a1;
        ArgWrap<A2> a2;
        std::shared_ptr<ResultCellAny> out;

        Task2(Callable f, A1 x1, A2 x2, std::shared_ptr<ResultCellAny> o)
            : fn(std::move(f)), a1(std::move(x1)), a2(std::move(x2)), out(std::move(o)) {}

        void execute() override {
            if (out->ready) { return; }
            out->value = Any(fn(a1.get(), a2.get()));
            out->ready = true;
        }

        bool ready() const override { return out->ready; }
        std::shared_ptr<ResultCellAny> output() const override { return out; }

        std::vector<std::shared_ptr<ResultCellAny>> deps() const override {
            std::vector<std::shared_ptr<ResultCellAny>> d;
            if (auto c1 = a1.depCell()) { d.push_back(std::move(c1)); }
            if (auto c2 = a2.depCell()) { d.push_back(std::move(c2)); }
            return d;
        }
    };

    template <class C>
    static C* objPtr(C& r) { return &r; }
    template <class C>
    static const C* objPtr(const C& r) { return &r; }
    template <class C>
    static C* objPtr(C* p) { return p; }
    template <class C>
    static const C* objPtr(const C* p) { return p; }
    template <class C>
    static C* objPtr(const std::shared_ptr<C>& p) { return p.get(); }
    template <class C>
    static const C* objPtr(const std::shared_ptr<const C>& p) { return p.get(); }

public:
    struct TopoExec {
        bool ok = false;
        std::vector<size_t> order;
        std::vector<size_t> stuck;
    };

    TTaskScheduler() = default;

    size_t size() const { return tasks_.size(); }

    template <class Callable>
    TaskId add(Callable fn) {
        auto out = std::make_shared<ResultCellAny>();
        auto t = std::make_shared<Task0<Callable>>(std::move(fn), out);
        bindProducer(out, t);
        tasks_.push_back(t);
        return TaskId(out);
    }

    template <class Callable, class A1>
    TaskId add(Callable fn, A1 a1) {
        auto out = std::make_shared<ResultCellAny>();
        auto t = std::make_shared<Task1<Callable, A1>>(std::move(fn), std::move(a1), out);
        bindProducer(out, t);
        tasks_.push_back(t);
        return TaskId(out);
    }

    template <class Callable, class A1, class A2>
    TaskId add(Callable fn, A1 a1, A2 a2) {
        auto out = std::make_shared<ResultCellAny>();
        auto t = std::make_shared<Task2<Callable, A1, A2>>(std::move(fn), std::move(a1), std::move(a2), out);
        bindProducer(out, t);
        tasks_.push_back(t);
        return TaskId(out);
    }

    template <class C, class R, class Obj>
    TaskId add(R (C::*pmf)(), Obj obj) {
        auto bound = [pmf, obj]() -> R { return (const_cast<C*>(objPtr(obj))->*pmf)(); };
        return add(bound);
    }

    template <class C, class R, class Obj>
    TaskId add(R (C::*pmf)() const, Obj obj) {
        auto bound = [pmf, obj]() -> R { return (objPtr(obj)->*pmf)(); };
        return add(bound);
    }

    template <class C, class R, class Obj, class P1, class A1>
    TaskId add(R (C::*pmf)(P1), Obj obj, A1 a1) {
        auto bound = [pmf, obj](P1 x) -> R { return (const_cast<C*>(objPtr(obj))->*pmf)(x); };
        return add(bound, std::move(a1));
    }

    template <class C, class R, class Obj, class P1, class A1>
    TaskId add(R (C::*pmf)(P1) const, Obj obj, A1 a1) {
        auto bound = [pmf, obj](P1 x) -> R { return (objPtr(obj)->*pmf)(x); };
        return add(bound, std::move(a1));
    }

    template <class C, class R, class Obj, class P1, class P2, class A1, class A2>
    TaskId add(R (C::*pmf)(P1, P2), Obj obj, A1 a1, A2 a2) {
        auto bound = [pmf, obj](P1 x, P2 y) -> R { return (const_cast<C*>(objPtr(obj))->*pmf)(x, y); };
        return add(bound, std::move(a1), std::move(a2));
    }

    template <class C, class R, class Obj, class P1, class P2, class A1, class A2>
    TaskId add(R (C::*pmf)(P1, P2) const, Obj obj, A1 a1, A2 a2) {
        auto bound = [pmf, obj](P1 x, P2 y) -> R { return (objPtr(obj)->*pmf)(x, y); };
        return add(bound, std::move(a1), std::move(a2));
    }

    template <class T>
    Future<T> getFutureResult(const TaskId& id) const { return Future<T>(id.out); }

    template <class T>
    const T& getResult(const TaskId& id) const { Future<T> f(id.out); return f.get(); }

    TopoExec executeTopologicallyDetailed(bool preResolveExternalDeps = true) {
        std::unordered_map<const void*, size_t> indexByPtr;
        indexByPtr.reserve(tasks_.size());
        for (size_t i = 0; i < tasks_.size(); ++i) {
            indexByPtr[tasks_[i].get()] = i;
        }

        std::vector<std::vector<size_t>> adj(tasks_.size());
        std::vector<size_t> indeg(tasks_.size(), 0);

        for (size_t v = 0; v < tasks_.size(); ++v) {
            auto d = tasks_[v]->deps();
            std::unordered_set<size_t> seen;
            for (auto& depCell : d) {
                if (!depCell) { continue; }

                if (preResolveExternalDeps) {
                    auto prod0 = depCell->producer.lock();
                    if (!prod0) {
                        if (!depCell->ready) { depCell->ensureReady(); }
                        continue;
                    }
                    auto base0 = dynamic_cast<ITaskBase*>(prod0.get());
                    if (!base0) {
                        if (!depCell->ready) { depCell->ensureReady(); }
                        continue;
                    }
                    if (!indexByPtr.count(base0)) {
                        if (!depCell->ready) { depCell->ensureReady(); }
                        continue;
                    }
                }

                auto prod = depCell->producer.lock();
                if (!prod) { continue; }
                auto raw = dynamic_cast<ITaskBase*>(prod.get());
                auto it = indexByPtr.find(raw);
                if (it == indexByPtr.end()) { continue; }
                size_t u = it->second;
                if (seen.insert(u).second) {
                    adj[u].push_back(v);
                    ++indeg[v];
                }
            }
        }

        std::queue<size_t> q;
        for (size_t i = 0; i < indeg.size(); ++i) {
            if (indeg[i] == 0) { q.push(i); }
        }

        TopoExec res;
        res.order.reserve(tasks_.size());

        while (!q.empty()) {
            size_t u = q.front();
            q.pop();
            res.order.push_back(u);

            if (!tasks_[u]->ready()) {
                tasks_[u]->execute();
            }

            for (size_t v : adj[u]) {
                if (indeg[v] > 0) {
                    --indeg[v];
                    if (indeg[v] == 0) {
                        q.push(v);
                    }
                }
            }
        }

        if (res.order.size() != tasks_.size()) {
            res.ok = false;
            for (size_t i = 0; i < indeg.size(); ++i) {
                if (indeg[i] != 0) { res.stuck.push_back(i); }
            }
            return res;
        }

        res.ok = true;
        return res;
    }

    bool executeTopologically() {
        return executeTopologicallyDetailed().ok;
    }
};