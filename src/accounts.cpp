#include "accounts.h"
#include <QJsonDocument>
#include <QThread>
#include <QElapsedTimer>
#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#endif

namespace moly {
void Secrets::put(const QString &key,const QByteArray &value) {
#ifdef Q_OS_WIN
    auto name=("MolyLauncher/"+key).toStdWString(); CREDENTIALW c{}; c.Type=CRED_TYPE_GENERIC; c.TargetName=name.data(); c.CredentialBlobSize=DWORD(value.size()); c.CredentialBlob=reinterpret_cast<LPBYTE>(const_cast<char*>(value.constData())); c.Persist=CRED_PERSIST_LOCAL_MACHINE;
    if(!CredWriteW(&c,0)) throw Error("Windows Credential Manager could not save this secret.");
#else
    Q_UNUSED(key); Q_UNUSED(value); throw Error("Secure account storage is currently supported on Windows only.");
#endif
}
QByteArray Secrets::get(const QString &key) {
#ifdef Q_OS_WIN
    auto name=("MolyLauncher/"+key).toStdWString(); PCREDENTIALW c=nullptr; if(!CredReadW(name.c_str(),CRED_TYPE_GENERIC,0,&c)) return {};
    QByteArray value(reinterpret_cast<char*>(c->CredentialBlob),c->CredentialBlobSize); CredFree(c); return value;
#else
    Q_UNUSED(key); return {};
#endif
}
void Secrets::remove(const QString &key) {
#ifdef Q_OS_WIN
    auto name=("MolyLauncher/"+key).toStdWString(); CredDeleteW(name.c_str(),CRED_TYPE_GENERIC,0);
#else
    Q_UNUSED(key);
#endif
}
LoginResult Accounts::exchange(const QJsonObject &oauth) {
    if(oauth["access_token"].toString().isEmpty()) throw Error("Microsoft sign-in did not return an access token.");
    auto xbox=net.json("https://user.auth.xboxlive.com/user/authenticate",{{"Properties",QJsonObject{{"AuthMethod","RPS"},{"SiteName","user.auth.xboxlive.com"},{"RpsTicket","d="+oauth["access_token"].toString()}}},{"RelyingParty","http://auth.xboxlive.com"},{"TokenType","JWT"}}).object();
    if(xbox["Token"].toString().isEmpty()) throw Error("Xbox sign-in failed. Check that this Microsoft account has an Xbox profile.");
    auto xsts=net.json("https://xsts.auth.xboxlive.com/xsts/authorize",{{"Properties",QJsonObject{{"SandboxId","RETAIL"},{"UserTokens",QJsonArray{xbox["Token"]}}}},{"RelyingParty","rp://api.minecraftservices.com/"},{"TokenType","JWT"}}).object();
    auto users=xsts["DisplayClaims"].toObject()["xui"].toArray(); if(users.isEmpty() || xsts["Token"].toString().isEmpty()) throw Error("Xbox authorization failed. Check account age restrictions and Xbox profile setup.");
    auto user=users[0].toObject(); QString identity="XBL3.0 x="+user["uhs"].toString()+';'+xsts["Token"].toString();
    auto mc=net.json("https://api.minecraftservices.com/authentication/login_with_xbox",{{"identityToken",identity}}).object();
    auto token=mc["access_token"].toString(); if(token.isEmpty()) throw Error("Minecraft authentication failed. This application's client ID may need Minecraft API approval.");
    QMap<QByteArray,QByteArray> headers{{"Authorization","Bearer "+token.toUtf8()}};
    auto ownership=net.json("https://api.minecraftservices.com/entitlements/mcstore",{},headers).object();
    if(ownership["items"].toArray().isEmpty()) throw Error("This account does not currently have a Minecraft Java Edition entitlement.");
    auto profile=net.json("https://api.minecraftservices.com/minecraft/profile",{},headers).object();
    if(profile["id"].toString().isEmpty() || profile["name"].toString().isEmpty()) throw Error("Minecraft profile not found. Finish setting up Java Edition on minecraft.net.");
    QJsonObject account{{"id",profile["id"]},{"uuid",profile["id"]},{"name",profile["name"]},{"type","microsoft"},{"xuid",user["xid"]}};
    auto refreshToken=oauth["refresh_token"].toString(); if(!refreshToken.isEmpty()) Secrets::put("account/"+profile["id"].toString(),refreshToken.toUtf8());
    return {account,token};
}
LoginResult Accounts::signIn(const QString &clientId,const std::function<void(QString,QString)> &codeReady) {
    if(clientId.trimmed().isEmpty()) throw Error("Add your Microsoft application client ID in Settings → Connections first.");
    const QString base="https://login.microsoftonline.com/consumers/oauth2/v2.0/";
    auto device=net.form(base+"devicecode",{{"client_id",clientId},{"scope","XboxLive.signin offline_access"}});
    if(device["device_code"].toString().isEmpty()) throw Error("Microsoft rejected this application. Enable public-client flows for your client ID.");
    codeReady(device["user_code"].toString(),device["verification_uri"].toString());
    QElapsedTimer timer; timer.start(); int interval=qMax(5,device["interval"].toInt(5));
    while(timer.elapsed()<qint64(device["expires_in"].toInt(900))*1000) {
        for(int i=0;i<interval*10;++i) { net.ctx.check(); QThread::msleep(100); }
        auto result=net.form(base+"token",{{"grant_type","urn:ietf:params:oauth:grant-type:device_code"},{"client_id",clientId},{"device_code",device["device_code"].toString()}},true);
        auto error=result["error"].toString(); if(error=="authorization_pending") continue; if(error=="slow_down") { interval+=5; continue; }
        if(!error.isEmpty()) throw Error(error=="authorization_declined"?"Sign-in was declined.":"Microsoft sign-in ended: "+error+". Try again.");
        return exchange(result);
    } throw Error("The sign-in code expired. Please try again.");
}
LoginResult Accounts::refresh(const QString &clientId,const QJsonObject &account) {
    auto token=Secrets::get("account/"+account["id"].toString()); if(token.isEmpty()) throw Error("This account needs to sign in again. You can also choose Play offline for an installed instance.");
    auto result=net.form("https://login.microsoftonline.com/consumers/oauth2/v2.0/token",{{"grant_type","refresh_token"},{"client_id",clientId},{"refresh_token",QString::fromUtf8(token)},{"scope","XboxLive.signin offline_access"}},true);
    if(result.contains("error")) throw Error("Your Microsoft session expired. Sign in again in Accounts, or explicitly choose Play offline.");
    auto refreshed=exchange(result); if(refreshed.account["id"]!=account["id"]) throw Error("Refreshed account did not match the selected profile."); return refreshed;
}
}
