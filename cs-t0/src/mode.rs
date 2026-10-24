use std::collections::BTreeSet;

use serde::{Deserialize, Serialize};

pub const OFFLINE_CALLBACK_THRESHOLD: u32 = 60;

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "PascalCase")]
#[repr(u32)]
pub enum NetworkMode {
    Unresolved = 0,
    Offline = 1,
    OriginalOnline = 2,
    CustomAuthorityServer = 3,
    CustomAuthorityClient = 4,
    Retired = 5,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum RequestedMode {
    Offline,
    OriginalOnline,
    CustomAuthorityServer,
    CustomAuthorityClient,
}

impl RequestedMode {
    #[must_use]
    pub const fn live_allowed(self) -> bool {
        matches!(self, Self::Offline | Self::OriginalOnline)
    }

    #[must_use]
    pub const fn frozen_mode(self) -> NetworkMode {
        match self {
            Self::Offline => NetworkMode::Offline,
            Self::OriginalOnline => NetworkMode::OriginalOnline,
            Self::CustomAuthorityServer => NetworkMode::CustomAuthorityServer,
            Self::CustomAuthorityClient => NetworkMode::CustomAuthorityClient,
        }
    }
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct SignalObservation {
    pub lua_generation_bound: bool,
    pub simulation_callback: bool,
    pub manager_identity: bool,
    pub stable_roster: bool,
    pub channel_zero_activity: bool,
    pub nonzero_channel: bool,
    pub wrong_thread: bool,
    pub manager_changed: bool,
    pub frame_regressed: bool,
    pub lua_closed: bool,
    pub returned_to_menu: bool,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ModeTransition {
    None,
    Frozen(NetworkMode),
    Retired,
}

#[derive(Clone, Debug)]
pub struct ModeLatch {
    state: NetworkMode,
    requested: RequestedMode,
    generation: u64,
    token: Option<u64>,
    retired_tokens: BTreeSet<u64>,
    lua_generation_bound: bool,
    simulation_callbacks: u32,
    manager_observations: u32,
    roster_observations: u32,
    channel_zero_calls: u32,
}

impl ModeLatch {
    #[must_use]
    pub fn new(requested: RequestedMode) -> Self {
        Self {
            state: NetworkMode::Unresolved,
            requested,
            generation: 0,
            token: None,
            retired_tokens: BTreeSet::new(),
            lua_generation_bound: false,
            simulation_callbacks: 0,
            manager_observations: 0,
            roster_observations: 0,
            channel_zero_calls: 0,
        }
    }

    #[must_use]
    pub const fn state(&self) -> NetworkMode {
        self.state
    }

    #[must_use]
    pub const fn generation(&self) -> u64 {
        self.generation
    }

    #[must_use]
    pub const fn token(&self) -> Option<u64> {
        self.token
    }

    pub fn begin_generation(&mut self, token: u64) -> Result<(), &'static str> {
        if token == 0 {
            return Err("generation token must be nonzero");
        }
        if self.retired_tokens.contains(&token) {
            return Err("retired generation token cannot be revived");
        }
        if self.state != NetworkMode::Unresolved && self.state != NetworkMode::Retired {
            return Err("active generation cannot be replaced");
        }
        if self.state == NetworkMode::Unresolved && self.token.is_some() {
            return Err("unresolved generation is already established");
        }
        self.generation = self
            .generation
            .checked_add(1)
            .ok_or("generation counter exhausted")?;
        self.state = NetworkMode::Unresolved;
        self.token = Some(token);
        self.lua_generation_bound = false;
        self.simulation_callbacks = 0;
        self.manager_observations = 0;
        self.roster_observations = 0;
        self.channel_zero_calls = 0;
        if !self.requested.live_allowed() {
            self.retire();
            return Err("custom authority modes are disarmed during T0-02 live runs");
        }
        Ok(())
    }

    pub fn observe(&mut self, signal: SignalObservation) -> ModeTransition {
        if self.state == NetworkMode::Retired || self.token.is_none() {
            return ModeTransition::None;
        }
        if signal.wrong_thread
            || signal.manager_changed
            || signal.frame_regressed
            || signal.lua_closed
            || signal.returned_to_menu
            || signal.nonzero_channel
        {
            self.retire();
            return ModeTransition::Retired;
        }

        self.lua_generation_bound |= signal.lua_generation_bound;
        self.simulation_callbacks = self
            .simulation_callbacks
            .saturating_add(u32::from(signal.simulation_callback));
        self.manager_observations = self
            .manager_observations
            .saturating_add(u32::from(signal.manager_identity));
        self.roster_observations = self
            .roster_observations
            .saturating_add(u32::from(signal.stable_roster));
        self.channel_zero_calls = self
            .channel_zero_calls
            .saturating_add(u32::from(signal.channel_zero_activity));

        if self.state != NetworkMode::Unresolved {
            let expected = self.requested.frozen_mode();
            if self.state != expected {
                self.retire();
                return ModeTransition::Retired;
            }
            return ModeTransition::None;
        }

        let online_signal_count = u8::from(self.manager_observations >= 2)
            + u8::from(self.roster_observations >= 2)
            + u8::from(self.channel_zero_calls > 0);
        let offline_ready = self.lua_generation_bound
            && self.simulation_callbacks >= OFFLINE_CALLBACK_THRESHOLD
            && self.manager_observations == 0
            && self.roster_observations == 0
            && self.channel_zero_calls == 0;

        match self.requested {
            RequestedMode::OriginalOnline if online_signal_count >= 2 => {
                self.state = NetworkMode::OriginalOnline;
                ModeTransition::Frozen(self.state)
            }
            RequestedMode::Offline if online_signal_count >= 2 => {
                self.retire();
                ModeTransition::Retired
            }
            RequestedMode::Offline if offline_ready => {
                self.state = NetworkMode::Offline;
                ModeTransition::Frozen(self.state)
            }
            RequestedMode::OriginalOnline if offline_ready => {
                self.retire();
                ModeTransition::Retired
            }
            RequestedMode::CustomAuthorityServer | RequestedMode::CustomAuthorityClient => {
                self.retire();
                ModeTransition::Retired
            }
            _ => ModeTransition::None,
        }
    }

    pub fn retire(&mut self) {
        if let Some(token) = self.token.take() {
            self.retired_tokens.insert(token);
        }
        self.state = NetworkMode::Retired;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn begin(requested: RequestedMode) -> ModeLatch {
        let mut latch = ModeLatch::new(requested);
        latch.begin_generation(7).unwrap();
        latch
    }

    #[test]
    fn freezes_original_online_only_after_two_corroborating_signals() {
        let mut latch = begin(RequestedMode::OriginalOnline);
        assert_eq!(
            latch.observe(SignalObservation {
                manager_identity: true,
                ..Default::default()
            }),
            ModeTransition::None
        );
        assert_eq!(
            latch.observe(SignalObservation {
                manager_identity: true,
                ..Default::default()
            }),
            ModeTransition::None
        );
        assert_eq!(
            latch.observe(SignalObservation {
                channel_zero_activity: true,
                ..Default::default()
            }),
            ModeTransition::Frozen(NetworkMode::OriginalOnline)
        );
    }

    #[test]
    fn freezes_offline_after_positive_run_window() {
        let mut latch = begin(RequestedMode::Offline);
        for _ in 0..OFFLINE_CALLBACK_THRESHOLD - 1 {
            assert_eq!(
                latch.observe(SignalObservation {
                    lua_generation_bound: true,
                    simulation_callback: true,
                    ..Default::default()
                }),
                ModeTransition::None
            );
        }
        assert_eq!(
            latch.observe(SignalObservation {
                lua_generation_bound: true,
                simulation_callback: true,
                ..Default::default()
            }),
            ModeTransition::Frozen(NetworkMode::Offline)
        );
    }

    #[test]
    fn conflicts_retire_and_never_hot_switch() {
        for signal in [
            SignalObservation {
                wrong_thread: true,
                ..Default::default()
            },
            SignalObservation {
                manager_changed: true,
                ..Default::default()
            },
            SignalObservation {
                frame_regressed: true,
                ..Default::default()
            },
            SignalObservation {
                lua_closed: true,
                ..Default::default()
            },
            SignalObservation {
                returned_to_menu: true,
                ..Default::default()
            },
            SignalObservation {
                nonzero_channel: true,
                ..Default::default()
            },
        ] {
            let mut latch = begin(RequestedMode::OriginalOnline);
            assert_eq!(latch.observe(signal), ModeTransition::Retired);
            assert_eq!(latch.state(), NetworkMode::Retired);
        }
    }

    #[test]
    fn retired_token_cannot_revive_but_new_generation_can_begin() {
        let mut latch = begin(RequestedMode::Offline);
        latch.retire();
        assert!(latch.begin_generation(7).is_err());
        latch.begin_generation(8).unwrap();
        assert_eq!(latch.generation(), 2);
        assert_eq!(latch.state(), NetworkMode::Unresolved);
    }

    #[test]
    fn custom_modes_are_defined_but_live_disarmed() {
        for requested in [
            RequestedMode::CustomAuthorityServer,
            RequestedMode::CustomAuthorityClient,
        ] {
            let mut latch = ModeLatch::new(requested);
            assert!(latch.begin_generation(9).is_err());
            assert_eq!(latch.state(), NetworkMode::Retired);
        }
    }
}
