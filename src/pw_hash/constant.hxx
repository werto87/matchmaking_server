#ifndef EB95EBA8_1297_44AA_9C1C_A1AB22A88B30
#define EB95EBA8_1297_44AA_9C1C_A1AB22A88B30

#include <sodium.h>

#ifdef DEBUG
auto const hash_memory = crypto_pwhash_argon2id_MEMLIMIT_MIN;
auto const hash_opt = crypto_pwhash_argon2id_OPSLIMIT_MIN;
#else
// TODO-TEMPLATE increase opt and memory so it take longer to get the clear passwords if an attacker steals the hashed passwords
// this will slow down login and create account but give security if the database gets stolen
auto const hash_memory = crypto_pwhash_argon2id_MEMLIMIT_MIN;
auto const hash_opt = crypto_pwhash_argon2id_OPSLIMIT_MIN;
#endif

#endif /* EB95EBA8_1297_44AA_9C1C_A1AB22A88B30 */
