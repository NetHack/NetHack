# HanNetHack 한국어 번역 계획

## 현재 진행 상황 (2026-01-18)

### 번역 통계
- **PO 파일 번역 완료**: 3,953개 문자열
- **조사 패턴 적용**: 1,296개 문자열에 `{은/는}`, `{이/가}`, `{을/를}` 등 적용

---

## 완료된 작업

### 1. 소스 코드 수정

#### pline.c
- [x] 조사 처리 함수 `ko_process_string()` 통합
- [x] `You()` 함수 - 한국어일 때 "You " 접두사 제거
- [x] `Your()` 함수 - 한국어일 때 "Your " 접두사 제거
- [x] `You_feel()` 함수 - 한국어일 때 접두사 제거
- [x] `You_cant()` 함수 - 한국어일 때 "You can't " 접두사 제거
- [x] `pline_The()` 함수 - 한국어일 때 "The " 접두사 제거
- [x] `There()` 함수 - 한국어일 때 "There " 접두사 제거
- [x] `You_hear()` 함수 - 한국어일 때 접두사 제거

#### polyself.c
- [x] `body_part()` 함수에 `_()` 적용하여 신체 부위 번역 활성화

#### dungeon.c
- [x] `surface()` 함수 - 모든 반환 문자열에 `_()` 적용 (floor, ground, wall, altar 등)
- [x] `ceiling()` 함수 - 모든 반환 문자열에 `_()` 적용 (ceiling, sky 등)

#### read.c
- [x] "cogitate", "pronounce" 동사에 `_()` 적용

### 2. PO 파일 번역 추가

#### 신체 부위 (66개)
- arm, eye, face, finger, fingertip, foot, hand, head, leg, neck, spine, toe, hair, blood, lung, nose, stomach
- pseudopod, forelimb, foreclaw, wing, tentacle, claw, trunk 등

#### 지형/표면 (20개)
- floor, ground, wall, ceiling, altar, fountain, stairs, bridge, ice, sky, doorway 등

#### 동사 (2개)
- cogitate (마음속으로 읽다)
- pronounce (발음하다)

### 3. 조사 패턴 수정 (7개)
| 원본 | 수정 |
|------|------|
| `%s를` | `%s{을/를}` |
| `%s을` | `%s{을/를}` |
| `%s이다` | `%s{이다/다}` |
| `%s이` | `%s{이/가}` |
| `%s로` | `%s{으로/로}` |

---

## 남은 작업

### 우선순위 1: 소스 코드 함수 수정

#### 아직 수정 안 된 pline 계열 함수
- [ ] `You_see()` - 한국어일 때 접두사 제거 필요
- [ ] 기타 You 계열 함수 확인

#### mbodypart() 함수
- [ ] 몬스터 신체 부위도 번역되도록 `_()` 적용 필요
- 위치: `src/polyself.c`
- 많은 반환문이 있어서 작업량 많음

#### 기타 문자열 반환 함수들
- [ ] `hliquid()` 함수 - "water", "lava" 등
- [ ] 기타 지형/물체 이름 반환 함수들

### 우선순위 2: 정적 문자열 번역 (81개)

`_()` 마크가 없는 정적 문자열들:
```c
// src/apply.c:311
static const char hollow_str[] = "a hollow sound.  This must be a secret %s!";

// src/apply.c:2220
static const char you_buy_it[] = "You tin it, you bought it!";

// src/insight.c:43
static const char You_[] = "You ", are[] = "are ", were[] = "were ";
```

**해결 방법**: 사용 시점에서 `_()` 로 감싸거나, 정적 정의를 제거하고 직접 번역 문자열 사용

### 우선순위 3: dat 폴더 데이터 파일

| 파일 | 줄 수 | 설명 |
|------|-------|------|
| rumors.tru | 374 | 참 소문 |
| rumors.fal | 397 | 거짓 소문 |
| epitaph.txt | 401 | 묘비명 |
| oracles.txt | 104 | 오라클 메시지 |
| engrave.txt | 93 | 각인 메시지 |
| data.base | 6,528 | 몬스터/아이템 설명 |

**총 약 8,000줄 번역 필요**

### 우선순위 4: 복수형/관사 처리

영어의 복수형 "s" 접미사, 관사 "a/an/the" 등이 직접 문자열로 사용되는 곳:
```c
(num > 1L) ? "s" : ""
(number_leashed() > 1) ? "a" : "the"
```

한국어에서는 이런 처리가 필요 없으므로 조건부로 제거 필요

---

## 조사 선택 시스템

### 지원되는 조사 패턴 (`ko_postpos.c`)
| 패턴 | 받침 O | 받침 X | 설명 |
|------|--------|--------|------|
| `{은/는}` | 은 | 는 | 주제 |
| `{이/가}` | 이 | 가 | 주격 |
| `{을/를}` | 을 | 를 | 목적격 |
| `{과/와}` | 과 | 와 | 접속 |
| `{으로/로}` | 으로 | 로 | 방향 (ㄹ받침→로) |
| `{아/야}` | 아 | 야 | 호격 |
| `{이다/다}` | 이다 | 다 | 서술격 |
| `{이었/였}` | 이었 | 였 | 과거 서술격 |

### 동작 원리
1. `pline()` 등에서 포맷팅 후 `ko_process_string()` 호출
2. `{X/Y}` 패턴을 찾아 앞 글자의 받침 확인
3. 적절한 조사로 치환

### 영어/숫자 지원
- 숫자: 1,3,6,7 (받침O), 8 (ㄹ받침), 0,2,4,5,9 (받침X)
- 알파벳: F,L,M,N,R,S,X (받침O), 나머지 (받침X)

---

## 파일 위치

- **PO 파일**: `po/ko.po`
- **MO 파일**: `messages.mo`
- **조사 처리**: `src/ko_postpos.c`, `include/ko_postpos.h`
- **i18n 초기화**: `src/i18n.c`, `include/i18n.h`

---

## 빌드 및 테스트

```bash
# MO 파일 생성
msgfmt -o messages.mo po/ko.po

# 통계 확인
msgfmt --statistics po/ko.po

# 중복 제거
msguniq po/ko.po -o po/ko.po
```

---

## 주의사항

1. **문장 구조 차이**: 영어 SVO vs 한국어 SOV
   - 영어: "You hit the monster"
   - 한국어: "몬스터를 때렸다"

2. **접두사 함수**: `You()`, `pline_The()` 등은 한국어에서 접두사 불필요

3. **동사 조합**: 동사가 `%s`로 삽입될 때 한국어 어순 주의

4. **정적 배열**: C의 정적 초기화에서는 `_()`를 직접 사용할 수 없음
