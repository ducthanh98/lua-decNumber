#include "../redismodule.h"
#include "../rmutil/util.h"
#include "../rmutil/strings.h"
#include "../rmutil/test_util.h"

#define  DECNUMDIGITS 34

#include "decNumber.h"


int decrementCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {

    if (argc != 4) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModule_AutoMemory(ctx);

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
    if (RedisModule_KeyType(key) != REDISMODULE_KEYTYPE_HASH &&
        RedisModule_KeyType(key) != REDISMODULE_KEYTYPE_EMPTY) {
        return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    }

    RedisModuleCallReply *currentValueReply = RedisModule_Call(ctx, "HGET", "ss", argv[1], argv[2]);
    RMUTIL_ASSERT_NOERROR(ctx, currentValueReply);

    RedisModuleString *currentValueRedisString = RedisModule_CreateStringFromCallReply(currentValueReply);
    if (!currentValueRedisString) {
        return 0;
    }
    const char *currentValueString = RedisModule_StringPtrLen(currentValueRedisString, NULL);
    const char *decrementValueString = RedisModule_StringPtrLen(argv[3], NULL);

    decNumber currentNum, decrementNum;
    decContext set;
    char resultStr[DECNUMDIGITS + 14];
    decContextDefault(&set, DEC_INIT_BASE);
    set.traps = 0;
    set.digits = DECNUMDIGITS;

    decNumberFromString(&currentNum, currentValueString, &set);
    decNumberFromString(&decrementNum, decrementValueString, &set);

    decNumber resultNum;
    decNumberSubtract(&resultNum, &currentNum, &decrementNum, &set);

    if (!decNumberIsNegative(&resultNum)) {
        decNumberToString(&resultNum, resultStr);
        RedisModuleCallReply *srep = RedisModule_Call(ctx, "HSET", "ssc", argv[1], argv[2], resultStr);
        RMUTIL_ASSERT_NOERROR(ctx, srep);

        RedisModule_ReplyWithStringBuffer(ctx, resultStr, strlen(resultStr));
        return REDISMODULE_OK;
    }

    if (RedisModule_CallReplyType(currentValueReply) == REDISMODULE_REPLY_NULL) {
        RedisModule_ReplyWithNull(ctx);
        return REDISMODULE_OK;
    }

    RedisModule_ReplyWithSimpleString(ctx, "NOP");
    return REDISMODULE_OK;
}


int RedisModule_OnLoad(RedisModuleCtx *ctx) {
    if (RedisModule_Init(ctx, "balance", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    RMUtil_RegisterWriteCmd(ctx, "balance.decrement", decrementCommand);
    return REDISMODULE_OK;
}