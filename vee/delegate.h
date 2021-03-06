#ifndef _VEE_DELEGATE_H_
#define _VEE_DELEGATE_H_

#include <memory>
#include <map>
#include <vee/exl.h>
#include <vee/tupleupk.h>
#include <vee/lock.h>

namespace vee {

namespace delegate_impl {

template <typename FuncSig, typename _Function>
bool compare_function(const _Function& lhs, const _Function& rhs)
{
    using target_type = typename std::conditional
        <
        std::is_function<FuncSig>::value,
        typename std::add_pointer<FuncSig>::type,
        FuncSig
        > ::type;
    if (const target_type* lhs_internal = lhs.template target<target_type>())
    {
        if (const target_type* rhs_internal = rhs.template target<target_type>())
            return *rhs_internal == *lhs_internal;
    }
    return false;
}

template <typename FTy>
class compareable_function: public std::function < FTy >
{
public:
    using function_t = std::function<FTy>;
private:
    bool(*_type_holder)(const function_t&, const function_t&);
public:
    compareable_function() = default;
    explicit compareable_function(function_t& f):
        function_t(f),
        _type_holder(compare_function< FTy, function_t >)
    {

    }
    explicit compareable_function(function_t&& f):
        function_t(std::move(f)),
        _type_holder(compare_function< FTy, function_t >)
    {

    }
    template <typename CallableObj> explicit compareable_function(CallableObj&& f):
        function_t(std::forward<CallableObj>(f)),
        _type_holder(compare_function< std::remove_reference<CallableObj>::type, function_t >)
    {
        // empty
    }
    template <typename CallableObj> compareable_function& operator=(CallableObj&& f)
    {
        function_t::operator =(std::forward<CallableObj>(f));
        _type_holder = compare_function < std::remove_reference<CallableObj>::type, function_t >;
        return *this;
    }
    friend bool operator==(const compareable_function& lhs, const compareable_function& rhs)
    {
        return rhs._type_holder(lhs, rhs);
    }
    /*friend bool operator==(const compareable_function &lhs, const function_t &rhs)
    {
        return rhs == lhs;
    }*/
};

} // !namespace delegate_impl

template <class FTy, 
          class LockTy = lock::empty_lock, 
          class UsrKeyTy = int32_t >
class delegate
{

};

template <class RTy, 
          class ...Args, 
          class LockTy, 
          class UsrKeyTy >
class delegate < RTy(Args ...), LockTy, UsrKeyTy >
{
/* Define types and constant variables */
/* Define Public types */
public:
    using this_t = delegate<RTy(Args...), LockTy>;
    using ref_t = this_t&;
    using rref_t = this_t&&;
    using shared_ptr = std::shared_ptr<this_t>;
    using unique_ptr = std::shared_ptr<this_t>;
    using args_tuple_t = std::tuple<std::remove_reference_t<Args>...>;
    using lock_t = LockTy;
    using key_t = void*;
    using usrkey_t = UsrKeyTy;
    using binder_t = std::function<RTy(Args ...)>;

/* Define Private types */
private:
    using _cmpbinder_t = delegate_impl::compareable_function<RTy(Args ...)>;
    using _key_cmpbinder_pair = std::pair<usrkey_t, binder_t>;
    using _seq_container_t = std::multimap<key_t, _cmpbinder_t>;
    using _usr_container_t = std::map<usrkey_t, binder_t>;
    
/* Define Public static member functions */
public:
    inline static key_t gen_key(binder_t& dst)
    {
        using target_type = typename std::conditional
            <
            std::is_function<RTy(Args...)>::value,
            typename std::add_pointer<RTy(Args...)>::type,
            RTy(Args...)
            > ::type;
        auto ptr = dst.template target<target_type>();
        if (!ptr)
            throw key_generation_failed_exception();
        //printf("wrapper: %X, key: %X\n", ptr, *ptr);
        return mpl::pvoid_cast(*ptr);
    }
    inline static key_t gen_key(binder_t&& dst)
    {
        return gen_key(dst);
    }
    template <class CallableObj>
    inline static key_t gen_key(CallableObj&& obj)
    {
        return gen_key(binder_t(std::forward<CallableObj>(obj)));
    }
    template <class UsrKeyRef>
    inline static mpl::type_to_type<usrkey_t> usrkey(UsrKeyRef&& key)
    {
        return mpl::type_to_type<usrkey_t>{ std::forward<UsrKeyRef>(key) };
    }
/* Define Public member functions */
public:
    delegate() = default;
    ~delegate() = default;
    explicit delegate(const ref_t other)
    {
        std::lock_guard<lock_t> locker(other._mtx);
        _cont = other._cont;
        _usrcont = other._usrcont;
    }
    delegate(rref_t other) noexcept
    {
        std::lock_guard<lock_t> locker(other._mtx);
        _cont = std::move(other._cont);
        _usrcont = std::move(other._usrcont);
    }
    ref_t operator=(const ref_t other)
    {
        // lock both mutexes without deadlock
        std::lock(_mtx, other._mtx);
        // make sure both already-locked mutexes are unlocked at the end of scope
        std::lock_guard<lock_t> locker1{ _mtx, std::adopt_lock };
        std::lock_guard<lock_t> locker2{ other._mtx, std::adopt_lock };

        _cont = other._cont;
        _usrcont = other._usrcont;

        return *this;
    }
    ref_t operator=(rref_t other) noexcept
    {
        // lock both mutexes without deadlock
        std::lock(_mtx, other._mtx);
        // make sure both already-locked mutexes are unlocked at the end of scope
        std::lock_guard<lock_t> locker1{ _mtx, std::adopt_lock };
        std::lock_guard<lock_t> locker2{ other._mtx, std::adopt_lock };

        _cont = std::move(other._cont);
        _usrcont = std::move(other._usrcont);

        return *this;
    }
    /*friend static this_t operator+(const ref_t lhs, const ref_t rhs)
    {
        this_t ret{ lhs };
        std::lock_guard<lock_t> locker{ rhs._mtx };
        ret += rhs;
        return ret;
    }*/
    ref_t operator+=(const ref_t rhs)
    {
        // lock both mutexes without deadlock
        std::lock(_mtx, rhs._mtx);
        // make sure both already-locked mutexes are unlocked at the end of scope
        std::lock_guard<lock_t> locker1{ _mtx, std::adopt_lock };
        std::lock_guard<lock_t> locker2{ rhs._mtx, std::adopt_lock };

        for (auto& it : rhs._cont)
        {
            _cont.insert(it);
        }
        for (auto& it : rhs._usrcont)
        {
            _usrcont.insert(it);
        }

        return *this;
    }
    ref_t operator+=(rref_t rhs)
    {
        // lock both mutexes without deadlock
        std::lock(_mtx, rhs._mtx);
        // make sure both already-locked mutexes are unlocked at the end of scope
        std::lock_guard<lock_t> locker1{ _mtx, std::adopt_lock };
        std::lock_guard<lock_t> locker2{ rhs._mtx, std::adopt_lock };

        for (auto& it : rhs._cont)
        {
            _cont.insert(std::move(it));
        }
        for (auto& it : rhs._usrcont)
        {
            _usrcont.insert(std::move(it));
        }

        return *this;
    }
    template <class CallableObj>
    explicit delegate(CallableObj&& obj)
    {
        _cmpbinder_t cmpbinder{ std::forward<CallableObj>(obj) };
        _cont.insert(std::make_pair(gen_key(cmpbinder), std::move(cmpbinder)));
    }
    template <class UsrKeyRef, class CallableObj>
    explicit delegate(UsrKeyRef&& key, CallableObj&& obj)
    {
        _usrcont.insert(std::make_pair(std::forward<UsrKeyRef>(key), std::forward<CallableObj>(obj)));
    }
    template <class URef>
    ref_t operator+=(URef&& uref)
    {
        return this->_register(std::forward<URef>(uref), mpl::binary_dispatch< mpl::is_pair<typename std::remove_reference<URef>::type >::value>());
    }
    template <class PairRef>
    ref_t _register(PairRef&& pair, mpl::binary_dispatch<true>/*is_pair == true*/)
    {
        std::lock_guard<lock_t> locker{ _mtx };
        using PairTy = typename std::remove_reference<PairRef>::type;
        using UsrKeyRef = typename std::conditional< std::is_rvalue_reference<PairRef>::value,
            usrkey_t&&,
            usrkey_t >::type;
        using CallableObjRef = typename std::conditional< std::is_rvalue_reference<PairRef>::value,
            typename PairTy::second_type&&,
            typename PairTy::second_type >::type;
        binder_t binder{ static_cast<CallableObjRef>(pair.second) };
        auto ret = _usrcont.insert(std::make_pair(static_cast<UsrKeyRef>(pair.first), binder));
        if (ret.second == false)
            throw key_already_exist_exception();
        return *this;
    }
    template <class CallableObj>
    ref_t _register(CallableObj&& obj, mpl::binary_dispatch<false>/*is_pair == false*/)
    {
        std::lock_guard<lock_t> locker{ _mtx };
        _cmpbinder_t cmpbinder{ std::forward<CallableObj>(obj) };
        _cont.insert(std::make_pair(gen_key(cmpbinder), std::move(cmpbinder)));
        return *this;
    }
    template <class CallableObj>
    ref_t operator-=(CallableObj&& obj)
    {
        std::lock_guard<lock_t> locker{ _mtx };
        key_t key = gen_key( binder_t{ std::forward<CallableObj>(obj) });
        auto  target = _cont.find(key);
        if (target == _cont.end())
            throw target_not_found_exception();
        _cont.erase(target);
        return *this;
    }
    ref_t operator-=(mpl::type_to_type<usrkey_t>& wrapped_key)
    {
        std::lock_guard<lock_t> locker{ _mtx };
        auto target = _usrcont.find(wrapped_key.value);
        if (target == _usrcont.end())
            throw target_not_found_exception{};
        _usrcont.erase(target);
        return *this;
    }
    ref_t operator-=(mpl::type_to_type<usrkey_t>&& wrapped_key)
    {
        return operator-=(wrapped_key);
    }
    void operator()(args_tuple_t& args)
    {
        std::lock_guard<lock_t> locker{ _mtx };
        for (auto& it : _cont)
        {
            tupleupk(static_cast<binder_t&>(it.second), args);
        }
        for (auto& it : _usrcont)
        {
            tupleupk(static_cast<binder_t&>(it.second), args);
        }
    }
    void operator()(args_tuple_t&& args)
    {
        std::lock_guard<lock_t> locker{ _mtx };
        for (auto& it : _cont)
        {
            tupleupk(static_cast<binder_t&>(it.second), static_cast<args_tuple_t&&>(args));
        }
        for (auto& it : _usrcont)
        {
            tupleupk(static_cast<binder_t&>(it.second), static_cast<args_tuple_t&&>(args));
        }
    }

    template <typename ...FwdArgs>
    void operator()(FwdArgs&& ...args)
    {
        std::lock_guard<lock_t> locker{ _mtx };
        for (auto& it : _cont)
        {
            it.second.operator()(std::forward<FwdArgs>(args)...);
        }
        for (auto& it : _usrcont)
        {
            it.second.operator()(std::forward<FwdArgs>(args)...);
        }
    }
    
    template <typename ...FwdArgs>
    inline void do_call(FwdArgs&& ...args)
    {
        this->operator()(std::forward<FwdArgs>(args)...);
    }

    void clear()
    {
        std::lock_guard<lock_t> locker{ _mtx };
        _cont.clear();
        _usrcont.clear();
    }
    bool empty() const
    {
        std::lock_guard<lock_t> locker{ _mtx };
        if (_cont.empty() && _usrcont.empty())
            return true;
        return false;
    }

/* Define Protected static member functions */
protected:
    inline static key_t gen_key(_cmpbinder_t& binder)
    {
        return gen_key(static_cast<binder_t&>(binder));
    }
    inline static key_t gen_key(_cmpbinder_t&& binder)
    {
        return gen_key(static_cast<binder_t&&>(binder));
    }

/* Define Protected member variables */
protected:
    mutable lock_t _mtx;
    _seq_container_t _cont;
    _usr_container_t _usrcont;
};

template <class FTy,
          class LockTy = lock::empty_lock,
          class UsrKeyTy = int32_t,
          class CallableObj >
              delegate<FTy, LockTy, UsrKeyTy> make_delegate(CallableObj&& func)
{
    return delegate<FTy, LockTy, UsrKeyTy>{ std::forward<CallableObj>(func) };

}

template <class FTy,
class LockTy = lock::empty_lock,
class UsrKeyTy = int32_t >
    delegate<FTy, LockTy, UsrKeyTy > make_delegate(std::function<FTy> func)
{
    return delegate<FTy, LockTy, UsrKeyTy >{ (func) };
}

} // !namespace vee

#endif // !_VEE_DELEGATE_H_
