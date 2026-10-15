#pragma once
#include "ll/api/event/Event.h"

class Actor;
class ActorDamageSource;

namespace ll::event {

class ActorHurtEvent : public Event {
private:
    Actor* mActor;
    const ActorDamageSource* mSource;
    float mDamage;
    bool mKnock;
    bool mIgnite;

public:
    ActorHurtEvent(Actor* actor, const ActorDamageSource* source, float damage, bool knock = true, bool ignite = false)
        : mActor(actor), mSource(source), mDamage(damage), mKnock(knock), mIgnite(ignite) {}

    Actor* getActor() const { return mActor; }
    const ActorDamageSource* getSource() const { return mSource; }
    float getDamage() const { return mDamage; }
    void setDamage(float dmg) { mDamage = dmg; }
    bool shouldKnock() const { return mKnock; }
    void setKnock(bool knock) { mKnock = knock; }
    bool shouldIgnite() const { return mIgnite; }
    void setIgnite(bool ignite) { mIgnite = ignite; }
};

class ActorDieEvent : public Event {
private:
    Actor* mActor;
    const ActorDamageSource* mSource;

public:
    ActorDieEvent(Actor* actor, const ActorDamageSource* source)
        : mActor(actor), mSource(source) {}

    Actor* getActor() const { return mActor; }
    const ActorDamageSource* getSource() const { return mSource; }
};

} // namespace ll::event
