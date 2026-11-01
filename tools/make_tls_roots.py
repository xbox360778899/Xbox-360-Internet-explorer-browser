"""Export public trust anchors from the vendored Mozilla/certifi CA bundle.

No private keys are read. BearSSL verifies chain, hostname and certificate time.
The small DER reader only extracts Subject and SubjectPublicKeyInfo.
"""
from pathlib import Path
import ssl
import re

def der(data, offset=0):
    start=offset;tag=data[offset];offset+=1
    length=data[offset];offset+=1
    if length&128:
        n=length&127
        if not 1<=n<=4: raise ValueError('DER length')
        length=int.from_bytes(data[offset:offset+n],'big');offset+=n
    end=offset+length
    if end>len(data): raise ValueError('DER boundary')
    return tag, data[offset:end], data[start:end], end

def children(data):
    offset=0
    while offset<len(data):
        item=der(data,offset);yield item;offset=item[3]

def anchor(cert):
    tbs=list(children(der(cert)[1]))[0][1]
    fields=list(children(tbs));i=1 if fields[0][0]==160 else 0
    subject=fields[i+4][2];spki=list(children(fields[i+5][1]))
    alg=list(children(spki[0][1]));oid=alg[0][1].hex();key=spki[1][1]
    if not key or key[0]!=0: raise ValueError('Unaligned SPKI')
    key=key[1:]
    if oid=='2a864886f70d010101':
        nums=list(children(der(key)[1]))
        return subject,1,nums[0][1].lstrip(b'\0'),nums[1][1].lstrip(b'\0')
    if oid=='2a8648ce3d0201':
        curve={'2a8648ce3d030107':23,'2b81040022':24,'2b81040023':25}.get(alg[1][1].hex())
        if curve:return subject,2,key,curve
    return None

def main():
    rows=[];out=['// Generated public root DNs and public keys; no private keys.', '#pragma once']
    seen=set()
    def array(name,data):
        out.append('static unsigned char '+name+'[]={'+','.join('0x%02x'%x for x in data)+'};')
    pem=(Path(__file__).resolve().parents[1]/'vendor/certifi/cacert.pem').read_text('ascii')
    certificates=re.findall(r'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----',pem,re.S)
    for block in certificates:
        cert=ssl.PEM_cert_to_DER_cert(block)
        if cert in seen:continue
        seen.add(cert)
        item=anchor(cert)
        if not item:continue
        dn,kind,key,param=item;n='hubRoot'+str(len(rows))
        array(n+'DN',dn);array(n+'Key',key)
        if kind==1:
            array(n+'Exp',param)
            pub='{BR_KEYTYPE_RSA, {{'+n+'Key,sizeof('+n+'Key),'+n+'Exp,sizeof('+n+'Exp)}}}'
        else:
            # Union starts with RSA storage; populate EC keys at runtime below.
            pub='{0, {{0,0,0,0}}}'
        rows.append((n,kind,param,pub))
    if not rows:raise RuntimeError('No usable TLS roots in vendored CA bundle')
    out.append('static br_x509_trust_anchor hubTrustAnchors[]={')
    for n,kind,param,pub in rows:
        out.append('{{'+n+'DN,sizeof('+n+'DN)},BR_X509_TA_CA,'+pub+'},')
    out.extend(['};','static void hubInitTrustAnchors(){'])
    for i,(n,kind,param,pub) in enumerate(rows):
        if kind==2:
            out.append('hubTrustAnchors[%d].pkey.key_type=BR_KEYTYPE_EC;'%i)
            out.append('hubTrustAnchors[%d].pkey.key.ec.curve=%d;'%(i,param))
            out.append('hubTrustAnchors[%d].pkey.key.ec.q=%sKey;'%(i,n))
            out.append('hubTrustAnchors[%d].pkey.key.ec.qlen=sizeof(%sKey);'%(i,n))
    out.extend(['}','static const size_t hubTrustAnchorCount=sizeof(hubTrustAnchors)/sizeof(hubTrustAnchors[0]);'])
    target=Path(__file__).resolve().parents[1]/'src/tls_roots.h'
    target.write_text('\n'.join(out)+'\n','ascii')
    print('TLS trust anchors:',len(rows))

if __name__=='__main__':main()
