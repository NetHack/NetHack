# NetHack 한국어 번역 가이드

## 개요

이 디렉토리는 NetHack의 한국어 번역 파일들을 관리합니다.

## 파일 구조

```
po/
├── Makefile           # 번역 빌드 규칙
├── translate-tool.sh  # 번역 관리 도구
├── nethack.pot        # 원문 템플릿 (자동 생성)
├── ko.po              # 한국어 번역 파일
├── ko.mo              # 컴파일된 번역 파일 (자동 생성)
└── README.md          # 이 문서
```

## 번역 워크플로우

### 1. POT 템플릿 생성

소스 코드에서 번역 대상 문자열을 추출합니다:

```bash
cd po
make pot
```

### 2. PO 파일 업데이트

POT 템플릿에서 새로운 문자열을 ko.po에 병합합니다:

```bash
make update-po
```

### 3. 번역 작업

ko.po 파일을 편집하여 번역합니다. 추천 도구:

- **poedit** (GUI): https://poedit.net/
- **lokalize** (KDE)
- **gtranslator** (GNOME)
- 텍스트 에디터 (VSCode + gettext 확장)

### 4. 번역 컴파일

ko.mo 파일을 생성합니다:

```bash
make compile
```

### 5. 통계 확인

```bash
make stats
# 또는
./translate-tool.sh stats
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

1. `ko.po` 파일 수정
2. `./translate-tool.sh validate`로 검증
3. `make compile`로 컴파일 테스트
4. Pull Request 제출

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
