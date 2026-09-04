# simple-ldapd project status

**Version:** 1.0.0  
**Status:** Production-usable single-host SSO directory via LDAP bind  
**Progress:** 0.x milestone series complete; 1.0.0 contract cut  
**Last updated:** September 2026  

## Complete

- Milestones 1–15 LDAP feature series
- PDU / session / idle limits for public-port hardening
- systemd / launchd units aligned with install paths
- GitHub Actions CI (`cmake` + `ctest` + cppcheck)

## Next

- Maintain and harden (patch releases as needed)
- Later: MIT GSSAPI, computed `memberOf`, replication (not in 1.0)
