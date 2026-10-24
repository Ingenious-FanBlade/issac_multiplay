#pragma once
namespace cs {
// tx-04-04 queue fixture only. No tick, participants or transport exist yet.
// The next transaction replaces this with the prototype AuthoritySession.
struct PlaceholderSession {
    bool hosting=false;
    bool Host() noexcept { if(hosting)return false;hosting=true;return true; }
    void Stop() noexcept { hosting=false; }
};
}
