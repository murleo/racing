#include "track.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include "default_strategy.h"
#include "my_strategy.h"

#define sqr(a) (a)*(a)

const int Track::RUN_TIME = 300;
const float Track::EPS = 1e-3;

Track::Track(float length, float width)
: m_run_finished(false), m_world(length, width)
{}

Track::Track(const json& r)
: m_world(r["world"])
{
    if (!deserialize(r)) {
        throw std::runtime_error("Track creation failed!");
        exit(1);  
    }
}

bool Track::run() {
    m_run_finished = false;
    
    if (m_world.getCockroaches().empty()) {
        throw std::invalid_argument("List of cockroaches cannot be empty!");
        return false;
    }
    
    // std::vector<int> standings(m_world.getCockroaches().size());
    // for (unsigned int i = 0; i < standings.size(); ++i) standings[i] = i;   
    // std::random_shuffle(standings.begin(), standings.end());
    // m_standings = standings;
    
    std::vector<IStrategy*> strategies;
    for (unsigned int i = 0; i < m_world.getCockroaches().size(); ++i) {
        if (m_world.getCockroaches()[i].getStrategyName() == MyStrategy::NAME)
            strategies.push_back((IStrategy*)new MyStrategy());
        else
            strategies.push_back((IStrategy*)new DefaultStrategy());
    }
    
    std::vector< std::vector<State> > allStates;
    std::vector<int> finished(m_world.getCockroaches().size(), 0);
    std::vector<int> standings;
        
    allStates.push_back(m_world.getStates());
    for (unsigned int i = 0; i < RUN_TIME; ++i) {
        std::vector<Move> moves(strategies.size());
        for (unsigned int j = 0; j < strategies.size(); ++j)
            if (!finished[j])
                strategies[j]->move(m_world.getCockroaches()[j], m_world, moves[j]);
        for (unsigned int j = 0; j < strategies.size(); ++j)
            if (!finished[j])
                m_world.update(moves[j], j);
        
        std::vector<State> s = m_world.getStates();
        allStates.push_back(s);
    
        for (unsigned int j = 0; j < s.size(); ++j)
            if (!finished[j] && s[j].getPos().first + EPS >= m_world.getLength()) {
                finished[j] = true;
                standings.push_back(j);
            }
    }
    
    int finishedCnt = standings.size();
    standings.resize(m_world.getCockroaches().size());
    for (unsigned int i = finishedCnt; i < standings.size(); ++i) standings[i] = i;
    std::sort(standings.begin() + finishedCnt, standings.end(), std::bind(&Track::standingsCmp, this, std::placeholders::_1, std::placeholders::_2));
    m_standings = standings;

    for (unsigned int i = 0; i < m_world.getCockroaches().size(); ++i)
        delete strategies[i];
    
    return m_run_finished = true;
}

void Track::reset() {
    m_run_finished = false;
    m_standings.clear();
}

void Track::setCockroaches(const std::vector<Cockroach>& c) {
    m_world.setCockroaches(c);
}

void Track::setObstacles(const std::vector<Obstacle>& r) {
    m_world.setObstacles(r);
}

json Track::serialize() const {
    json r;
    r["run_finished"] = m_run_finished;
    r["world"] = m_world.serialize();
    if (m_run_finished)
        r["standings"] = m_standings;    
    return r;
}

bool Track::deserialize(const json& r) {
    try {
        m_run_finished = r["run_finished"];
        m_world = World(r["world"]);
        if (r["standings"].size() != m_world.getCockroaches().size())
            reset();
        else {
            m_standings.clear();
            for (unsigned int i = 0; i < r["standings"].size(); ++i)
                m_standings.push_back(r["standings"][i]);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return false;
    }
    return true;
}

bool Track::standingsCmp(const int& a, const int& b) {
    return m_world.getStates()[a].getPos().first < m_world.getStates()[b].getPos().first;
}