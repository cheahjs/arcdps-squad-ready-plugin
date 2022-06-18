use arcdps::{UserInfo, UserInfoIter, UserRole};
use log::*;
use std::collections::hash_map::Entry;
use std::collections::HashMap;

pub struct SquadMemberState {
    pub join_time: u64,
    pub role: UserRole,
    pub subgroup: u8,
    pub is_ready: bool,
}

impl SquadMemberState {
    fn new(join_time: u64, role: UserRole, subgroup: u8, is_ready: bool) -> Self {
        Self {
            join_time,
            role,
            subgroup,
            is_ready,
        }
    }

    fn update_user(&mut self, user_update: &UserInfo) {
        self.join_time = user_update.join_time;
        self.role = user_update.role;
        self.subgroup = user_update.subgroup;
        self.is_ready = user_update.ready_status;
    }
}

pub struct SquadTracker {
    self_account_name: String,
    squad_members: HashMap<String, SquadMemberState>,
}

impl SquadTracker {
    pub fn new(self_account_name: &str) -> Self {
        Self {
            self_account_name: String::from(self_account_name),
            squad_members: HashMap::new(),
        }
    }

    pub fn squad_update(
        &mut self,
        users: UserInfoIter,
        ready_check_callback: Option<fn()>,
        squad_ready_callback: Option<fn()>,
    ) {
        let SquadTracker {
            self_account_name,
            squad_members,
        } = &mut *self;

        debug!("Received {:?} updates", users.len());
        for user_update in users.into_iter() {
            debug!("User update: {:?}", user_update);

            let account_name = match user_update.account_name {
                Some(x) => x,
                None => continue,
            };

            match user_update.role {
                UserRole::SquadLeader | UserRole::Lieutenant | UserRole::Member => {
                    // User has been added or updated
                    let entry = squad_members.entry(account_name.to_string());
                    // Check if user has either:
                    // * Readied up if already exists
                    // * Is already ready if adding
                    let new_readied_user_state = match entry {
                        Entry::Occupied(entry) => {
                            let user = entry.into_mut();
                            let old_ready_status = user.is_ready;
                            user.update_user(&user_update);
                            if !old_ready_status && user.is_ready {
                                Some(user)
                            } else {
                                None
                            }
                        }
                        Entry::Vacant(entry) => {
                            let user = entry.insert(SquadMemberState::new(
                                user_update.join_time,
                                user_update.role,
                                user_update.subgroup,
                                user_update.ready_status,
                            ));
                            if user_update.ready_status == true {
                                Some(user)
                            } else {
                                None
                            }
                        }
                    };
                    if let Some(new_readied_user_state) = new_readied_user_state {
                        if check_ready_status(new_readied_user_state) {
                            if let Some(ready_check_callback) = ready_check_callback {
                                ready_check_callback();
                            }
                        }
                    }
                }
                UserRole::None => {
                    if account_name == self_account_name {
                        squad_members.clear();
                    } else {
                        squad_members.remove(account_name);
                    }
                }
                UserRole::Invited | UserRole::Applied | UserRole::Invalid => {}
            };

            match user_update.role {
                UserRole::SquadLeader
                | UserRole::Lieutenant
                | UserRole::Member
                | UserRole::None => {
                    // Check if the entire squad is ready if a member of the squad is updated
                    if check_all_ready(squad_members) {
                        if let Some(ready_callback) = squad_ready_callback {
                            ready_callback();
                        }
                    }
                }
                _ => {}
            }
        }
    }
}

// Returns if a ready check has been started.
// A ready check is considered started if the user is a squad leader, and that they have just readied up.
fn check_ready_status(user: &mut SquadMemberState) -> bool {
    if user.role != UserRole::SquadLeader {
        return false;
    }
    user.is_ready
}

// Returns if everyone in the squad is ready and a ready check is completed.
fn check_all_ready(squad_members: &mut HashMap<String, SquadMemberState>) -> bool {
    let mut readied_users: Vec<&String> = Vec::new();
    let squad_member_count = squad_members.len();

    for (account_name, state) in squad_members.iter() {
        if state.is_ready {
            readied_users.push(account_name);
        } else {
            debug!("{} is not readied - {:?}", account_name, state.role);
        }
    }

    if readied_users.len() == squad_member_count {
        debug!(
            "All {} users readied - ready check done",
            squad_member_count
        );
        return true;
    }
    debug!(
        "{}/{} users readied",
        readied_users.len(),
        squad_member_count
    );
    false
}
