# NetHack 한국어 번역 가이드

## 개요

이 디렉토리는 NetHack의 한국어 번역 파일들을 관리합니다.

## 파일 구조

```
po/
├── ko_manual.po       # ⭐ 수동 번역 (이 파일을 편집!)
├── ko.po              # 자동 추출 (편집 금지!)
├── ko_merged.po       # 병합 결과 (자동 생성)
├── ko.mo              # 컴파일된 바이너리 (자동 생성)
├── nethack.pot        # 원문 템플릿 (자동 생성)
├── Makefile           # 번역 빌드 규칙
└── README.md          # 이 문서
```

## ⚠️ 중요: 번역 파일 정책

### 반드시 `ko_manual.po`를 편집하세요!

| 파일 | 역할 | 편집 | 위험성 |
|------|------|------|--------|
| `ko_manual.po` | 수동 번역 | ⭐ **O** | 없음 (안전) |
| `ko.po` | 자동 추출 | ❌ **X** | `update-po` 시 덮어쓰기 가능 |
| `ko_merged.po` | 병합 결과 | ❌ X | 자동 생성됨 |

### 왜 ko.po를 편집하면 안 되나요?

1. `make update-po` 실행 시 소스에서 문자열을 다시 추출
2. 기존 번역이 보존되지만, 구조가 바뀌면 손실 가능
3. `ko_manual.po`는 절대 덮어쓰이지 않음!

### 병합 우선순위

```
ko_manual.po (우선) + ko.po (보조) → ko_merged.po → ko.mo
```

동일한 msgid가 있으면 `ko_manual.po`의 번역이 사용됩니다.

---

## 번역 워크플로우

### 일반 번역 작업 (권장)

```bash
cd po

# 1. ko_manual.po 편집 (수동 번역 추가/수정)
vi ko_manual.po

# 2. 병합 + 컴파일
make compile

# 3. 설치
make install DESTDIR=../dat
```

### 소스에서 새 문자열 추출할 때

```bash
cd po

# 1. POT 템플릿 생성
make pot

# 2. 안전하게 ko.po 업데이트 (백업 자동 생성)
make safe-update

# 3. 새 문자열을 ko_manual.po에 번역 추가
vi ko_manual.po

# 4. 병합 + 컴파일
make compile
```

### 번역 통계 확인

```bash
make stats
```

## 번역 규칙

### 조사 처리

한국어의 조사는 앞 단어의 받침에 따라 달라집니다. 특수 패턴을 사용하세요:

| 패턴 | 받침 O | 받침 X | 용도 |
|------|--------|--------|------|
| `{은/는}` | 은 | 는 | 주제격 |
| `{이/가}` | 이 | 가 | 주격 |
| `{을/를}` | 을 | 를 | 목적격 |
| `{과/와}` | 과 | 와 | 접속 |
| `{으로/로}` | 으로 | 로* | 방향/도구 |
| `{아/야}` | 아 | 야 | 호격 |
| `{이다/다}` | 이다 | 다 | 서술격 |

*ㄹ받침은 "로" 사용

**예시:**

```
msgid "You hit %s."
msgstr "%s{을/를} 때렸다."
```

결과:
- "고블린**을** 때렸다." (받침 있음)
- "오크**를** 때렸다." (받침 없음)

### 형식 지정자

원문의 `%s`, `%d` 등은 반드시 번역문에도 포함해야 합니다:

```
msgid "You have %d gold pieces."
msgstr "금화 %d개를 가지고 있다."
```

순서 변경이 필요한 경우 위치 지정:

```
msgid "%s hits %s."
msgstr "%2$s{을/를} %1$s{이/가} 때렸다."
```

### 문체 가이드

1. **'-다' 체 사용** (해요체 X)
   - ✓ "때렸다", "죽였다"
   - ✗ "때렸어요", "죽였습니다"

2. **간결한 표현**
   - ✓ "기분이 나아졌다."
   - ✗ "당신의 기분이 좋아진 것 같습니다."

3. **능동태 선호**
   - ✓ "고블린을 죽였다."
   - ✗ "고블린이 당신에 의해 죽임을 당했다."

### 용어 통일

| 영어 | 한국어 |
|------|--------|
| hit | 때리다 |
| miss | 빗맞다 |
| kill | 죽이다 |
| destroy | 파괴하다 |
| damage | 피해 |
| gold (piece) | 금화 |
| experience | 경험치 |
| level | 레벨 |
| dungeon | 던전 |
| potion | 물약 |
| scroll | 두루마리 |
| wand | 지팡이 |
| armor | 갑옷 |
| weapon | 무기 |

## 번역 관리 도구

`translate-tool.sh`는 번역 작업을 도와주는 스크립트입니다:

```bash
# 통계 보기
./translate-tool.sh stats

# 문자열 검색
./translate-tool.sh search "You hit"

# 미번역 목록
./translate-tool.sh untranslated

# 검토 필요 목록
./translate-tool.sh fuzzy

# 조사 패턴 검증
./translate-tool.sh postpos-check

# CSV로 내보내기 (스프레드시트 검토용)
./translate-tool.sh export-csv

# 번역 유효성 검사
./translate-tool.sh validate

# 백업 생성
./translate-tool.sh backup
```

## 기여 방법

1. `ko_manual.po` 파일 편집 (ko.po가 아님!)
2. `make compile`로 컴파일 테스트
3. `make stats`로 통계 확인
4. Pull Request 제출

### ko_manual.po에 추가할 항목들

- 몬스터 이름 (`newt` → `도롱뇽`)
- 아이템 이름 (`long sword` → `장검`)
- 역할/종족 이름
- 포맷 문자열 어순 수정 (위치 지정자 사용)
- ko.po의 잘못된 번역 수정 (덮어쓰기)

## 테스트

번역 적용 테스트:

```bash
# 전체 빌드 후
cd ../..
make

# 한국어로 실행
LANG=ko_KR.UTF-8 ./nethack

# 또는 심볼셋과 함께
LANG=ko_KR.UTF-8 ./nethack -symset:Korean
```

## 문의

번역 관련 문의는 GitHub Issues를 이용해주세요.
