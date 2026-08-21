# EXI Grammar Terminology

## EXI Spec Terminology:

### Rules (Non-Terminals)
- **NT-0, NT-1, NT-2**, etc.
- Each NT is a **rule** = a state in the grammar state machine
- A grammar is a collection of these rules
- Also called "non-terminals" in formal grammar theory

### Productions (Events)
- **EE, CH, AT, SE**, etc.
- Each production is one **event choice** within a rule
- Productions are the possible events you can encode/decode when in that rule
- The number after each production is the **event code** (production index)

### Production Structure
```
CH [str] NT-1 1
│  │     │    └─ event code (production index within the rule)
│  │     └─ non-terminal (which rule to transition to next)
│  │     └─ type info (datatype for value encoding)
└─ event type (EE, CH, AT, SE, etc.)
```

### numberOfEvents
- **Count of productions** in a rule
- Determines **how many bits** to read for event code selection:
  - numberOfEvents = 1 → **0 bits** (implicit, only one choice)
  - numberOfEvents = 2 → **1 bit** (0 or 1)
  - numberOfEvents = 3-4 → **2 bits** (00, 01, 10, 11)
  - numberOfEvents = 5-8 → **3 bits**

### Example Grammar
```
Grammar 46 :USER_DEFINED
NT-0:                                      ← RULE 0
    AT ([0:7]:parameter) [str] NT-1  0     ← Production: AT event, transition to NT-1, code 0
    numberOfEvents = 1 → 0 bits

NT-1:                                      ← RULE 1  
    EE 0                                   ← Production: EE event, code 0
    CH [str] NT-1 1                        ← Production: CH event, self-loop to NT-1, code 1
    numberOfEvents = 2 → 1 bit (choose between EE or CH)
```
