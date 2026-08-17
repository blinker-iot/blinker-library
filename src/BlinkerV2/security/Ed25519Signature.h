#ifndef BLINKER_SECURITY_ED25519SIGNATURE_H
#define BLINKER_SECURITY_ED25519SIGNATURE_H

#include "../interface/IEd25519DigestVerifier.h"

namespace blinker {

bool isCanonicalEd25519PublicKey(ByteView publicKey);
bool isCanonicalEd25519Signature(ByteView signature);

} // namespace blinker

#endif
