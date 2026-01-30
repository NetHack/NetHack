# HanNetHack 번역 작업 완벽 가이드

## 목차
1. [프로젝트 개요](#1-프로젝트-개요)
2. [프로젝트 구조](#2-프로젝트-구조)
3. [번역 시스템 (gettext)](#3-번역-시스템-gettext)
4. [한국어 번역 규칙](#4-한국어-번역-규칙)
5. [소스 코드에서 문자열 래핑](#5-소스-코드에서-문자열-래핑)
6. [번역 파일 관리](#6-번역-파일-관리)
7. [조합 메시지 패턴 번역](#7-조합-메시지-패턴-번역)
8. [from_what() 원인 표시 처리](#8-from_what-원인-표시-처리)
9. [동사 접두어 상수 처리](#9-동사-접두어-상수-처리)
10. [주석 작성 규칙](#10-주석-작성-규칙)
11. [특수 번역 항목](#11-특수-번역-항목)
12. [도움말 및 데이터 파일 번역](#12-도움말-및-데이터-파일-번역)
13. [빌드 및 검증](#13-빌드-및-검증)
14. [테스트 방법](#14-테스트-방법)
15. [문제 해결](#15-문제-해결)
16. [전체 작업 흐름](#16-전체-작업-흐름)
17. [유용한 명령어 모음](#17-유용한-명령어-모음)

---

## 1. 프로젝트 개요

### 1.1 HanNetHack이란?
NetHack 3.7의 한국어 번역 프로젝트입니다. GNU gettext 기반 i18n 시스템을 사용합니다.

### 1.2 번역 범위
- **소스 코드 메시지**: 게임 내 모든 텍스트 (약 9,000+ 문자열)
- **도움말 파일**: help, cmdhelp, opthelp 등
- **데이터 파일**: rumors, epitaph, engrave 등
- **Lua 스크립트**: 퀘스트 대화, 던전 메시지 등

### 1.3 번역 원칙
- 간결하고 명확한 표현
- '-다' 체 사용 (해요체 X)
- 고유명사는 음역 또는 의역 선택
- 게임 용어 일관성 유지

---

## 2. 프로젝트 구조

```
/home/jinhong_kim/workspace/HanNetHack/
├── src/                          # C 소스 코드
│   ├── insight.c                 # enlightenment 메시지 (주요)
│   ├── pline.c                   # 메시지 출력
│   ├── dog.c                     # 펫 관련 (펫 이름 등)
│   ├── objnam.c                  # 아이템 이름
│   ├── monst.c                   # 몬스터 정의
│   ├── i18n.c                    # 국제화 함수
│   ├── ko_postpos.c              # 한국어 조사 처리
│   └── obj_descr_i18n.c          # 아이템 설명 번역
├── include/
│   ├── i18n.h                    # _(), N_() 매크로
│   └── ko_postpos.h              # 조사 처리 헤더
├── po/
│   ├── ko.po                     # 한국어 번역 (편집 대상)
│   ├── nethack.pot               # 번역 템플릿 (자동 생성)
│   ├── messages.mo               # 컴파일된 번역
│   ├── Makefile                  # 번역 빌드
│   └── TRANSLATION_GUIDE.md      # 이 가이드
├── dat/
│   ├── locale/ko/                # 한국어 데이터 파일
│   │   ├── help                  # 도움말
│   │   ├── cmdhelp               # 명령어 도움말
│   │   ├── rumors.tru            # 진짜 소문
│   │   ├── rumors.fal            # 가짜 소문
│   │   └── *.lua                 # Lua 스크립트
│   └── nhdat                     # 컴파일된 게임 데이터
└── win/
    ├── tty/                      # TTY 인터페이스
    └── curses/                   # Curses 인터페이스
```

---

## 3. 번역 시스템 (gettext)

### 3.1 작동 원리

```
소스 코드 (.c)  →  POT 파일  →  PO 파일  →  MO 파일  →  게임
   _("text")      (템플릿)      (번역)      (바이너리)
```

### 3.2 파일 역할

| 파일 | 역할 | 편집 |
|------|------|------|
| `*.c` | 소스 코드, _() 매크로로 래핑 | O |
| `nethack.pot` | 추출된 문자열 템플릿 | X |
| `ko.po` | 한국어 번역 | O |
| `messages.mo` | 컴파일된 바이너리 | X |

### 3.3 매크로 종류

```c
_("text")       // 런타임 번역 (일반적)
N_("text")      // 마킹만 (정적 배열용)
gettext("text") // _()와 동일
P_("s", "p", n) // 복수형 (한국어는 불필요)
```

---

## 4. 한국어 번역 규칙

### 4.1 조사 처리 시스템

한국어는 앞 글자의 받침에 따라 조사가 달라집니다.
`ko_postpos.c`에서 자동 처리됩니다.

```c
// 사용 예
pline(_("%s{이/가} 공격한다."), mon_nam(mtmp));
```

#### 지원되는 조사 패턴

| 패턴 | 받침 있음 | 받침 없음 | 용도 |
|------|----------|----------|------|
| `{은/는}` | 은 | 는 | 주제 |
| `{이/가}` | 이 | 가 | 주격 |
| `{을/를}` | 을 | 를 | 목적격 |
| `{과/와}` | 과 | 와 | 접속 |
| `{으로/로}` | 으로 | 로 | 방향/도구 |
| `{아/야}` | 아 | 야 | 호격 |
| `{이다/다}` | 이다 | 다 | 서술격 |
| `{이/}` | 이 | (없음) | 주격 (생략) |

#### 예시

```
msgid "You hit %s."
msgstr "%s{을/를} 때렸다."

msgid "%s attacks!"
msgstr "%s{이/가} 공격한다!"

msgid "with %s"
msgstr "%s{으로/로}"
```

### 4.2 문체 규칙

- **기본 문체**: '-다' 체 (해요체 X)
- **간결함**: 불필요한 조사/어미 생략 가능
- **일관성**: 같은 상황은 같은 표현

```
# 좋은 예
"배가 고프다."
"%s{을/를} 때렸다."

# 나쁜 예
"배가 고파요."
"%s를 때렸습니다."
```

### 4.3 용어 통일

| 영어 | 한국어 |
|------|--------|
| hit | 때리다 |
| kill | 죽이다 |
| miss | 빗나가다 |
| damage | 피해 |
| gold | 금화 |
| experience | 경험치 |
| level | 레벨 |
| dungeon | 던전 |
| monster | 몬스터 |
| armor | 갑옷 |
| weapon | 무기 |
| potion | 물약 |
| scroll | 두루마리 |
| wand | 마법봉 |
| ring | 반지 |
| amulet | 부적 |

### 4.4 고유명사 처리

#### 음역 (소리 그대로)
```
Yendor → 옌더
Gehennom → 게헨놈
Moloch → 몰록
Medusa → 메두사
```

#### 의역 (뜻 번역)
```
Amulet of Yendor → 옌더의 부적
Bell of Opening → 개봉의 종
Book of the Dead → 죽음의 책
```

---

## 5. 소스 코드에서 문자열 래핑

### 5.1 기본 래핑

```c
// 변경 전
pline("You feel hungry.");

// 변경 후
pline(_("You feel hungry."));
```

### 5.2 포맷 문자열

```c
// 변경 전
pline("You hit %s.", mon_nam(mtmp));

// 변경 후
pline(_("You hit %s."), mon_nam(mtmp));
```

### 5.3 정적 배열 (N_() 사용)

```c
// 변경 전
static const char *messages[] = { "opt1", "opt2" };

// 변경 후
static const char *messages[] = { N_("opt1"), N_("opt2") };
// 사용: pline("%s", _(messages[i]));
```

### 5.4 조건부 문자열

```c
// 변경 전
msg = cond ? "yes" : "no";

// 변경 후
msg = cond ? _("yes") : _("no");
```

### 5.5 enl_msg() 패턴

```c
// 변경 전
enl_msg(You_, "would fly", "would have flown", " if trapped", "");

// 변경 후
enl_msg(You_, _("would fly"), _("would have flown"), _(" if trapped"), "");
```

### 5.6 래핑하면 안 되는 것

```c
// 이미 번역된 문자열
pline("%s", translated_string);

// 디버그 메시지
impossible("debug message");

// 빈 문자열
enl_msg(You_, verb, verb, suffix, "");  // 마지막 ""
```

### 5.7 i18n.h 포함 확인

```c
#include "hack.h"
#include "i18n.h"  // 필수!
```

---

## 6. 번역 파일 관리

### 6.1 POT 파일 생성

```bash
cd /home/jinhong_kim/workspace/HanNetHack/po
make pot
```

### 6.2 PO 파일 업데이트

```bash
msgmerge -U ko.po nethack.pot
```

결과:
- 새 문자열: `msgstr ""` 추가
- 변경된 문자열: `#, fuzzy` 표시
- 삭제된 문자열: `#~` 주석 처리

### 6.3 PO 파일 구조

```
# 주석
#: ../src/file.c:123     # 소스 위치
#, c-format              # 포맷 플래그
#, fuzzy                 # 검토 필요
msgid "English"          # 원문
msgstr "한국어"           # 번역
```

### 6.4 fuzzy 처리

```bash
# fuzzy 찾기
grep -n "^#, fuzzy" ko.po

# fuzzy 개수
grep -c "^#, fuzzy" ko.po
```

처리: 번역 확인 후 `#, fuzzy` 줄 삭제

### 6.5 MO 파일 컴파일

```bash
msgfmt -c -v -o messages.mo ko.po
```

### 6.6 상태 확인

```bash
# 전체 통계
msgfmt -c -v -o /dev/null ko.po

# 미번역 보기
msgattrib --untranslated ko.po
```

---

## 7. 조합 메시지 패턴 번역

### 7.1 문제 상황

`enl_msg()` 함수가 문자열을 조합:

```c
enl_msg(prefix, present_verb, past_verb, suffix, ps);
// 결과: prefix + verb + suffix + ps
```

영어와 한국어 어순이 달라서 문제 발생.

### 7.2 해결 전략

#### 전략 1: 명사화 패턴 (권장)

```
영어: "You hunger rapidly"
한국어: 당신은 + "배고픔이" + " 빠르게 증가합니다"
결과: "당신은 배고픔이 빠르게 증가합니다"
```

#### 전략 2: 상태 표시 패턴

```
영어: "You fall asleep uncontrollably"
한국어: 당신은 + "잠듦:" + " 통제 불가"
결과: "당신은 잠듦: 통제 불가"
```

#### 전략 3: 의미 재배치

```
영어: "You have gone without food"
한국어: verb="음식 없이" suffix=" 지냈습니다"
결과: "당신은 음식 없이 지냈습니다"
```

### 7.3 주의사항

- 번역을 한쪽으로 합치지 말 것
- 각 부분에 의미 분산 유지
- from_what() 추가 고려

---

## 8. from_what() 원인 표시 처리

### 8.1 반환값 목록

| 영어 | 한국어 |
|------|--------|
| `" because of %s"` | `" (%s의 영향)"` |
| `" from birth"` | `" (선천적)"` |
| `" innately"` | `" (타고난 능력)"` |
| `" intrinsically"` | `" (내재적 능력)"` |
| `" because of your experience"` | `" (경험으로 획득)"` |
| `" due to your lycanthropy"` | `" (수인화의 영향)"` |
| `" from your creature form"` | `" (현재 형태의 영향)"` |

### 8.2 괄호 형식 이유

```
영어: "You hunger rapidly because of the ring"
한국어: "당신은 배고픔이 빠르게 증가합니다 (반지의 영향)"
```

괄호 형식은 어순과 무관하게 자연스러움.

---

## 9. 동사 접두어 상수 처리

### 9.1 목록

```c
#define are         _("are ")
#define were        _("were ")
#define have        _("have ")
#define had         _("had ")
#define can         _("can ")
#define could       _("could ")
```

### 9.2 처리 방법

**빈 문자열로 번역**:

```
msgid "are "
msgstr ""
```

이유: 한국어는 동사가 문장 끝에 옴.
의미는 suffix에서 완성.

### 9.3 주의

빈 문자열이 **정상**! 반드시 주석 필요.

---

## 10. 주석 작성 규칙

### 10.1 조합 메시지 주석

```
# ============================================================
# [조합 메시지] insight.c:1170 - "You hunger/hungered rapidly"
# 영어: You_ + "hunger"/"hungered" + " rapidly" + from_what()
# 한국어: 당신은 + "배고픔이" + " 빠르게 증가합니다" + [원인]
# from_what(): "" / " (선천적)" / " (%s의 영향)" 등
# 결과: "당신은 배고픔이 빠르게 증가합니다 (반지의 영향)"
# 주의: 개별 문자열만 보면 오역 같지만 조합하면 자연스러움
# ============================================================
```

### 10.2 동사 접두어 주석

```
# ============================================================
# [동사 접두어] you_are() 매크로용
# 한국어는 동사가 끝에 오므로 빈 문자열이 정상
# 예: "You are swimming" → "당신은 수영 중"
# 주의: 빈 문자열이 정상! 오역 아님!
# ============================================================
```

### 10.3 공유 문자열 주석

```
# 주의: 여러 곳에서 사용됨
# - insight.c:1652 (stealth)
# - insight.c:1884 (다른 컨텍스트)
# 두 컨텍스트 모두에서 작동해야 함
```

---

## 11. 특수 번역 항목

### 11.1 펫 이름 (dog.c)

```c
// src/dog.c
petname = _("Slasher");   // 베기꾼
petname = _("Hachi");     // 하치
petname = _("Idefix");    // 이데픽스
petname = _("Sirius");    // 시리우스
```

### 11.2 몬스터 이름

대부분 `monst.c`의 `mons[]` 배열에 정의.
`obj_descr_i18n.c`에서 번역 처리.

### 11.3 아이템 이름

`objects.c`의 `objects[]` 배열에 정의.
`obj_descr_i18n.c`에서 번역 처리.

### 11.4 직업/종족 이름

```
# 직업
Archeologist → 고고학자
Barbarian → 야만인
Caveman → 원시인
...

# 종족
Human → 인간
Elf → 엘프
Dwarf → 드워프
...
```

---

## 12. 도움말 및 데이터 파일 번역

### 12.1 도움말 파일 위치

```
dat/locale/ko/
├── help           # 일반 도움말
├── hh             # 짧은 도움말
├── cmdhelp        # 명령어 도움말
├── keyhelp        # 키 도움말
├── opthelp        # 옵션 도움말
├── optmenu        # 옵션 메뉴
├── wizhelp        # 마법사 도움말
├── usagehlp       # 사용법
└── history        # 역사
```

### 12.2 데이터 파일

```
dat/locale/ko/
├── rumors.tru     # 진짜 소문
├── rumors.fal     # 가짜 소문
├── epitaph.txt    # 묘비명
├── engrave.txt    # 새겨진 글
├── bogusmon.txt   # 가짜 몬스터
└── oracles.txt    # 신탁
```

### 12.3 Lua 파일

```
dat/locale/ko/*.lua
```

퀘스트 대화, 던전 설명 등.

### 12.4 데이터 파일 형식

#### rumors (소문)
```
진짜 소문 내용
%
가짜 소문은 rumors.fal에
```

#### epitaph (묘비)
```
여기 용감한 모험가가 잠들다
%
그는 용처럼 살았다
```

---

## 13. 빌드 및 검증

### 13.1 전체 빌드

```bash
# 1. 소스 빌드
cd /home/jinhong_kim/workspace/HanNetHack
make -j4

# 2. POT 생성
cd po && make pot

# 3. PO 업데이트
msgmerge -U ko.po nethack.pot

# 4. fuzzy 처리
grep -c "^#, fuzzy" ko.po

# 5. MO 컴파일
msgfmt -c -v -o messages.mo ko.po

# 6. 데이터 재생성
cd .. && make
```

### 13.2 번역만 업데이트

```bash
cd /home/jinhong_kim/workspace/HanNetHack/po
msgfmt -c -v -o messages.mo ko.po
cd .. && make
```

### 13.3 검증 체크리스트

- [ ] msgfmt 오류 없음
- [ ] fuzzy 0개
- [ ] 미번역 확인 (의도적인 것만)
- [ ] 포맷 지정자 일치
- [ ] 게임 실행 테스트

---

## 14. 테스트 방법

### 14.1 게임 실행

```bash
./src/nethack
```

### 14.2 wizard 모드

```bash
./src/nethack -D
```

테스트:
- `Ctrl+X`: 속성 확인
- `#conduct`: 행동 규칙
- `#wish`: 아이템 획득
- `#levelchange`: 레벨 변경

### 14.3 특정 메시지 테스트

1. wizard 모드 진입
2. 해당 상황 재현
3. 메시지 확인

---

## 15. 문제 해결

### 15.1 포맷 지정자 불일치

```
ko.po:1234: format specifications not same
```

해결: %s, %d 순서/개수 일치

### 15.2 인코딩 오류

```
invalid multibyte sequence
```

해결: UTF-8 확인

### 15.3 번역 미반영

1. MO 재컴파일
2. nhdat 재생성
3. 게임 재시작

### 15.4 공유 문자열 충돌

해결:
1. 소스에서 문자열 분리 (권장)
2. 두 컨텍스트 모두 작동하는 번역

---

## 16. 전체 작업 흐름

### 16.1 새 문자열 번역

```
1. 미번역 문자열 찾기
   msgattrib --untranslated ko.po

2. 소스 확인 (컨텍스트 파악)
   grep -n "문자열" ../src/*.c

3. 번역 추가
   에디터로 ko.po 편집

4. 컴파일 & 테스트
   msgfmt -c -v -o messages.mo ko.po
```

### 16.2 소스 코드 수정 (새 문자열 래핑)

```
1. 래핑 필요한 문자열 찾기
   grep 'pline("' ../src/file.c | grep -v '_('

2. _() 매크로로 래핑
   에디터로 소스 수정

3. 빌드
   cd .. && make -j4

4. POT 재생성
   cd po && make pot

5. PO 업데이트
   msgmerge -U ko.po nethack.pot

6. 새 문자열 번역
   에디터로 ko.po 편집

7. MO 컴파일 & 테스트
   msgfmt -c -v -o messages.mo ko.po
```

### 16.3 조합 메시지 작업

```
1. enl_msg 패턴 찾기
   grep "enl_msg" ../src/insight.c

2. _() 래핑 확인/추가

3. 패턴 분석
   - prefix, verb, suffix 조합 확인
   - from_what() 여부 확인

4. 번역 전략 결정
   - 명사화 / 상태표시 / 의미재배치

5. 번역 추가 + 주석

6. 테스트
   wizard 모드에서 해당 상황 재현
```

---

## 17. 유용한 명령어 모음

### 17.1 검색

```bash
# 소스 검색
grep -rn "문자열" ../src/

# enl_msg 패턴
grep "enl_msg" ../src/insight.c

# 래핑 안 된 문자열
grep 'pline("' ../src/*.c | grep -v '_('

# PO 검색
grep -A2 'msgid "문자열"' ko.po
```

### 17.2 통계

```bash
# 전체 상태
msgfmt -c -v -o /dev/null ko.po

# fuzzy 개수
grep -c "^#, fuzzy" ko.po

# 미번역 개수
msgattrib --untranslated ko.po | grep -c "^msgid"

# 총 문자열
grep -c "^msgid " ko.po
```

### 17.3 편집

```bash
# PO 업데이트
msgmerge -U ko.po nethack.pot

# MO 컴파일
msgfmt -c -v -o messages.mo ko.po

# 미번역 추출
msgattrib --untranslated ko.po > untranslated.po

# fuzzy 추출
msgattrib --only-fuzzy ko.po > fuzzy.po
```

### 17.4 빌드

```bash
# POT 생성
cd po && make pot

# 전체 빌드
cd /home/jinhong_kim/workspace/HanNetHack && make -j4

# 클린 빌드
make clean && make -j4
```

---

## 부록 A: 완성된 번역 예시

### A.1 조합 메시지

```
# ============================================================
# [조합 메시지] insight.c:2126 - "You have gone/went without food"
# 영어: You_ + "have gone"/"went" + " without food"
# 한국어: 당신은 + "음식 없이" + " 지냈습니다"
# 결과: "당신은 음식 없이 지냈습니다"
# ============================================================
#: ../src/insight.c:2126
msgid "have gone"
msgstr "음식 없이"

#: ../src/insight.c:2126
msgid "went"
msgstr "음식 없이"

#: ../src/insight.c:2126
msgid " without food"
msgstr " 지냈습니다"
```

### A.2 from_what()

```
# ============================================================
# [from_what()] 원인 표시 - 괄호 형식
# ============================================================
#: ../src/attrib.c:912
#, c-format
msgid " because of %s"
msgstr " (%s의 영향)"

#: ../src/attrib.c:935
msgid " from birth"
msgstr " (선천적)"
```

### A.3 동사 접두어

```
# ============================================================
# [동사 접두어] 빈 문자열이 정상
# ============================================================
#: ../src/insight.c:44
msgid "are "
msgstr ""
```

---

## 부록 B: 자주 묻는 질문

### Q: 빈 문자열 번역이 오류인가요?
A: 동사 접두어(are, have 등)는 빈 문자열이 정상입니다.
   한국어는 동사가 문장 끝에 오기 때문입니다.

### Q: fuzzy가 뭔가요?
A: 원문이 변경되어 번역 검토가 필요하다는 표시입니다.
   확인 후 `#, fuzzy` 줄을 삭제하세요.

### Q: 공유 문자열은 어떻게 처리하나요?
A: `#:` 줄에서 여러 파일이 나열되면 공유 문자열입니다.
   모든 컨텍스트에서 작동하는 번역을 선택하거나,
   소스에서 문자열을 분리하세요.

### Q: 조사({은/는})가 안 되는데요?
A: `ko_postpos.c`가 제대로 링크되었는지 확인하세요.
   `#include "i18n.h"`도 필요합니다.

---

*이 가이드는 HanNetHack 번역 작업의 모든 내용을 담고 있습니다.*
*다음 세션에서 이 파일을 참조하면 바로 작업을 시작할 수 있습니다.*
