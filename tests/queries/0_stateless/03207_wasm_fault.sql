SET allow_experimental_analyzer = 1;

DROP FUNCTION IF EXISTS huge_allocate;
DROP FUNCTION IF EXISTS access_data;
DROP FUNCTION IF EXISTS infinite_loop;
DROP FUNCTION IF EXISTS fib_wasm;
DROP FUNCTION IF EXISTS write_out_of_bounds;
DROP FUNCTION IF EXISTS read_out_of_bounds;
DROP FUNCTION IF EXISTS returns_out_of_bounds;
DROP FUNCTION IF EXISTS returns_out_of_bounds2;
DROP FUNCTION IF EXISTS test_func;

DELETE FROM system.webassembly_modules WHERE name = 'test_faulty';
DELETE FROM system.webassembly_modules WHERE name = 'big_module';
DELETE FROM system.webassembly_modules WHERE name = 'test_faulty_abiv1';

INSERT INTO system.webassembly_modules (name, code) SELECT 'test_faulty', base64Decode(concat(
    'AGFzbQEAAAABCQJgAABgAX8BfwMHBgABAQEBAQUDAQACBj8KfwFBgIgEC38AQYAIC38AQYAIC38AQYAIC38AQYCIBAt/AEGACAt/',
    'AEGAiAQLfwBBgIAIC38AQQALfwBBAQsH8gEQBm1lbW9yeQIAEV9fd2FzbV9jYWxsX2N0b3JzAAANaHVnZV9hbGxvY2F0ZQABDWlu',
    'ZmluaXRlX2xvb3AAAgNmaWIAAxN3cml0ZV9vdXRfb2ZfYm91bmRzAAQScmVhZF9vdXRfb2ZfYm91bmRzAAUMX19kc29faGFuZGxl',
    'AwEKX19kYXRhX2VuZAMCC19fc3RhY2tfbG93AwMMX19zdGFja19oaWdoAwQNX19nbG9iYWxfYmFzZQMFC19faGVhcF9iYXNlAwYK',
    'X19oZWFwX2VuZAMHDV9fbWVtb3J5X2Jhc2UDCAxfX3RhYmxlX2Jhc2UDCQqZAwYCAAsdAQF/QX8hAQNAIAFBAWohASAAQABBf0cN',
    'AAsgAQsRAAJAIAANAEEADwsDfwwACwtFAQF/AkAgAEUNAEEAIQECQCAAQQNJDQBBACEBA0AgAEF/ahCDgICAACABaiEBIABBfmoi',
    'AEECSw0ACwsgACABag8LAAALbgECfwJAIABFDQAgAEEHcSEBPwBBEHQhAgJAIABBCEkNACAAQXhxIQADQCACQoGChIiQoMCAATcA',
    'ACACQQhqIQIgAEF4aiIADQALCyABRQ0AA0AgAkEBOgAAIAJBAWohAiABQX9qIgENAAsLQQALrgEBBH8CQCAADQBBAA8LIABBB3Eh',
    'AT8AQRB0IQICQAJAIABBCE8NAEEAIQMMAQsgAEF4cSEEQQAhAyACIQADQCADIAAtAABqIAAtAAFqIAAtAAJqIAAtAANqIAAtAARq',
    'IAAtAAVqIAAtAAZqIAAtAAdqIQMgAEEIaiICIQAgBEF4aiIEDQALCwJAIAFFDQADQCADIAItAABqIQMgAkEBaiECIAFBf2oiAQ0A',
    'CwsgAwsAiQEEbmFtZQAMC2ZhdWx0eS53YXNtAWAGABFfX3dhc21fY2FsbF9jdG9ycwENaHVnZV9hbGxvY2F0ZQINaW5maW5pdGVf',
    'bG9vcAMDZmliBBN3cml0ZV9vdXRfb2ZfYm91bmRzBRJyZWFkX291dF9vZl9ib3VuZHMHEgEAD19fc3RhY2tfcG9pbnRlcgBnCXBy',
    'b2R1Y2VycwEMcHJvY2Vzc2VkLWJ5AQxVYnVudHUgY2xhbmdAMTguMS44ICgrKzIwMjQwNzMxMDI0OTQ0KzNiNWI1YzFlYzRhMy0x',
    'fmV4cDF+MjAyNDA3MzExNDUwMDAuMTQ0KQAsD3RhcmdldF9mZWF0dXJlcwIrD211dGFibGUtZ2xvYmFscysIc2lnbi1leHQ='
));

CREATE OR REPLACE FUNCTION huge_allocate LANGUAGE WASM ABI PLAIN FROM 'test_faulty' ARGUMENTS (UInt32) RETURNS UInt32 SETTINGS max_memory = 655360;
SELECT huge_allocate(1 :: UInt32) <= 10 SETTINGS send_logs_level = 'fatal';
SELECT huge_allocate(10 :: UInt32) == 0 SETTINGS send_logs_level = 'fatal';

CREATE OR REPLACE FUNCTION huge_allocate LANGUAGE WASM ABI PLAIN FROM 'test_faulty' ARGUMENTS (UInt32) RETURNS UInt32 SETTINGS max_memory = 6553600;
SELECT 10 < huge_allocate(1 :: UInt32) AND huge_allocate(1 :: UInt32) <= 100 SETTINGS send_logs_level = 'fatal';

CREATE OR REPLACE FUNCTION infinite_loop LANGUAGE WASM ABI PLAIN FROM 'test_faulty' ARGUMENTS (UInt32) RETURNS UInt32;
SELECT infinite_loop(1 :: UInt32); -- { serverError WASM_ERROR }

CREATE OR REPLACE FUNCTION infinite_loop LANGUAGE WASM ABI PLAIN FROM 'test_faulty' ARGUMENTS (UInt32) RETURNS UInt32 SETTINGS max_fuel = 100_000_000;
SELECT infinite_loop(1 :: UInt32); -- { serverError WASM_ERROR }

CREATE OR REPLACE FUNCTION infinite_loop LANGUAGE WASM ABI PLAIN FROM 'test_faulty' ARGUMENTS (UInt32) RETURNS UInt32 SETTINGS max_fuel = 1000;
SELECT infinite_loop(1 :: UInt32); -- { serverError WASM_ERROR }

CREATE OR REPLACE FUNCTION infinite_loop LANGUAGE WASM ABI PLAIN FROM 'test_faulty' ARGUMENTS (UInt32) RETURNS UInt32 SETTINGS max_fuel = 1_000_000_000_000; -- { serverError BAD_ARGUMENTS }
SELECT infinite_loop(1 :: UInt32); -- { serverError WASM_ERROR }

CREATE OR REPLACE FUNCTION fib_wasm LANGUAGE WASM ABI PLAIN FROM 'test_faulty' :: 'fib' ARGUMENTS (UInt32) RETURNS UInt32;
SELECT fib_wasm(5 :: UInt32);

SELECT fib_wasm((100 * number + 1) :: UInt32) FROM numbers(100000); -- { serverError WASM_ERROR }
SELECT fib_wasm(0 :: UInt32); -- { serverError WASM_ERROR }

DROP FUNCTION IF EXISTS fib_wasm;

CREATE OR REPLACE FUNCTION write_out_of_bounds LANGUAGE WASM ABI PLAIN FROM 'test_faulty' ARGUMENTS (UInt32) RETURNS UInt32;
SELECT write_out_of_bounds(number:: UInt32) FROM numbers(10); -- { serverError WASM_ERROR }
DROP FUNCTION IF EXISTS write_out_of_bounds;

CREATE OR REPLACE FUNCTION read_out_of_bounds LANGUAGE WASM ABI PLAIN FROM 'test_faulty' ARGUMENTS (UInt32) RETURNS UInt32;
SELECT read_out_of_bounds(number:: UInt32) FROM numbers(10); -- { serverError WASM_ERROR }
DROP FUNCTION IF EXISTS read_out_of_bounds;

INSERT INTO system.webassembly_modules (name, hash, code) SELECT 'big_module',
    reinterpretAsUInt256(unhex('f2baf4ab3d8ee8ac876c8b317cefc977ac76bf08ffedc6de176b06f9a77b6ecb')),
    base64Decode(concat(
        'AGFzbQEAAAABCQJgAABgAX8BfwMDAgABBQMBAEIGRwp/AUGAiIQCC38AQYAIC38AQYCIgAILfwBBgIiAAgt/AEGAiIQCC38AQYAI',
        'C38AQYCIhAILfwBBgICIAgt/AEEAC38AQQELB68BDAZtZW1vcnkCABFfX3dhc21fY2FsbF9jdG9ycwAAC2FjY2Vzc19kYXRhAAEM',
        'X19kc29faGFuZGxlAwEKX19kYXRhX2VuZAMCC19fc3RhY2tfbG93AwMMX19zdGFja19oaWdoAwQNX19nbG9iYWxfYmFzZQMFC19f',
        'aGVhcF9iYXNlAwYKX19oZWFwX2VuZAMHDV9fbWVtb3J5X2Jhc2UDCAxfX3RhYmxlX2Jhc2UDCQosAgIACycBAX9BfyEBAkAgAEH/',
        '/z9LDQAgAEECdEGAiICAAGooAgAhAQsgAQsLioCAAgEAQYAIC4CAgAIqAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA',
        'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA',
 repeat('AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA', 55921),
        'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA',
        'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFoEbmFtZQAQD2JpZ19tb2R1bGUud2FzbQEhAgAR',
        'X193YXNtX2NhbGxfY3RvcnMBC2FjY2Vzc19kYXRhBxIBAA9fX3N0YWNrX3BvaW50ZXIJCgEABy5yb2RhdGEAZwlwcm9kdWNlcnMB',
        'DHByb2Nlc3NlZC1ieQEMVWJ1bnR1IGNsYW5nQDE4LjEuOCAoKysyMDI0MDczMTAyNDk0NCszYjViNWMxZWM0YTMtMX5leHAxfjIw',
        'MjQwNzMxMTQ1MDAwLjE0NCkALA90YXJnZXRfZmVhdHVyZXMCKw9tdXRhYmxlLWdsb2JhbHMrCHNpZ24tZXh0'
));

CREATE OR REPLACE FUNCTION access_data LANGUAGE WASM ABI PLAIN FROM 'big_module' ARGUMENTS (UInt32) RETURNS Int32 SETTINGS max_memory = 655360;
SELECT access_data(0 :: UInt32) == 42; -- { serverError WASM_ERROR }

CREATE OR REPLACE FUNCTION access_data LANGUAGE WASM ABI PLAIN FROM 'big_module' ARGUMENTS (UInt32) RETURNS Int32 SETTINGS max_memory = 6553600;
SELECT access_data(0 :: UInt32);

INSERT INTO system.webassembly_modules (name, code) SELECT 'test_faulty_abiv1', base64Decode(concat(
    'AGFzbQEAAAABEwRgAABgAX8Bf2ABfwBgAn9/AX8DBwYAAQIDAwMFAwEAAgZRDX8BQaCoBAt/AEGQCAt/AEGACAt/AEGQKAt/AEGA',
    'CAt/AEGUKAt/AEGgKAt/AEGgqAQLfwBBgAgLfwBBoKgEC38AQYCACAt/AEEAC38AQQELB7wCEwZtZW1vcnkCABFfX3dhc21fY2Fs',
    'bF9jdG9ycwAAGGNsaWNraG91c2VfY3JlYXRlX2J1ZmZlcgABCWZyZWVfbGlzdAMBCGhlYXBfcG9zAwIZY2xpY2tob3VzZV9kZXN0',
    'cm95X2J1ZmZlcgACDWZyZWVfbGlzdF9wb3MDAwl0ZXN0X2Z1bmMAAxVyZXR1cm5zX291dF9vZl9ib3VuZHMABBZyZXR1cm5zX291',
    'dF9vZl9ib3VuZHMyAAUMX19kc29faGFuZGxlAwQKX19kYXRhX2VuZAMFC19fc3RhY2tfbG93AwYMX19zdGFja19oaWdoAwcNX19n',
    'bG9iYWxfYmFzZQMIC19faGVhcF9iYXNlAwkKX19oZWFwX2VuZAMKDV9fbWVtb3J5X2Jhc2UDCwxfX3RhYmxlX2Jhc2UDDAq0CAYC',
    'AAvtAQECf0EAIQECQAJAAkACQANAAkAgAUGQiICAAGooAgAiAkUNACACKAIEIABPDQILAkAgAUGUiICAAGooAgAiAkUNACACKAIE',
    'IABPDQMLIAFBCGoiAUGAIEcNAAtBACECAkBBACgCgIiAgAAiAQ0AQQBBAUAAQRB0IgE2AoCIgIAACyABIABqQQhqPwBBEHRLDQNB',
    'ACABQQhqIgI2AoCIgIAAIAEgAjYCACABIAA2AgRBAEEAKAKAiICAACAAajYCgIiAgAAgAQ8LIAFBkIiAgABqIQEMAQsgAUGUiICA',
    'AGohAQsgAUEANgIACyACC08BAn9BAEEAKAKQqICAAEEBakH/B3EiATYCkKiAgAACQAJAIAFBAnRBkIiAgABqIgEoAgAiAkUNACAC',
    'KAIEIAAoAgRPDQELIAEgADYCAAsLzwMBBn8gAUEBdCECQQAhAwJAAkACQAJAA0ACQCADQZCIgIAAaigCACIERQ0AIAQoAgQgAk8N',
    'AgsCQCADQZSIgIAAaigCACIERQ0AIAQoAgQgAk8NAwsgA0EIaiIDQYAgRw0AC0EAIQQCQEEAKAKAiICAACIDDQBBAEEBQABBEHQi',
    'AzYCgIiAgAALIAMgAmpBCGo/AEEQdEsNA0EAIANBCGoiBDYCgIiAgAAgAyAENgIAIAMgAjYCBEEAQQAoAoCIgIAAIAJqNgKAiICA',
    'ACADIQQMAwsgA0GQiICAAGohAwwBCyADQZSIgIAAaiEDCyADQQA2AgALAkAgAUUNACABQQFxIQUgACgCACEDIAQoAgAhAAJAIAFB',
    'AUYNACABQX5xIQZBACEHA0AgAy0AACECIABBCjoAASAAIAI6AAADQCADLQAAIQEgA0EBaiICIQMgAUEKRw0ACyACLQAAIQMgAEEK',
    'OgADIAAgAzoAAgNAIAItAAAhASACQQFqIgMhAiABQQpHDQALIABBBGohACAHQQJqIgcgBkcNAAsLIAVFDQAgAy0AACECIABBCjoA',
    'ASAAIAI6AAADQCADLQAAIQIgA0EBaiEDIAJBCkcNAAsLIAQLFQEBfz8AQRB0QXxqIgJBKjYCACACC4gCAQN/QYBgIQICQAJAAkAC',
    'QAJAA0ACQCACQZCogIAAaigCACIDRQ0AIAJBkKiAgABqIQIMBQsgAkGUqICAAGooAgAiAw0DIAJBmKiAgABqKAIAIgMNAiACQZyo',
    'gIAAaigCACIDDQEgAkEQaiICDQALQQAhAwJAQQAoAoCIgIAAIgINAEEAQQFAAEEQdCICNgKAiICAAAsgAkEIaiIEPwBBEHRLDQRB',
    'ACAENgKAiICAACACQQA2AgQgAiAENgIAIAIhAwwECyACQZyogIAAaiECDAILIAJBmKiAgABqIQIMAQsgAkGUqICAAGohAgsgAkEA',
    'NgIACyADQSo2AgQgAz8AQRB0NgIAIAMLAKwBBG5hbWUACwphYml2MS53YXNtAYMBBgARX193YXNtX2NhbGxfY3RvcnMBGGNsaWNr',
    'aG91c2VfY3JlYXRlX2J1ZmZlcgIZY2xpY2tob3VzZV9kZXN0cm95X2J1ZmZlcgMJdGVzdF9mdW5jBBVyZXR1cm5zX291dF9vZl9i',
    'b3VuZHMFFnJldHVybnNfb3V0X29mX2JvdW5kczIHEgEAD19fc3RhY2tfcG9pbnRlcgBnCXByb2R1Y2VycwEMcHJvY2Vzc2VkLWJ5',
    'AQxVYnVudHUgY2xhbmdAMTguMS44ICgrKzIwMjQwNzMxMDI0OTQ0KzNiNWI1YzFlYzRhMy0xfmV4cDF+MjAyNDA3MzExNDUwMDAu',
    'MTQ0KQAsD3RhcmdldF9mZWF0dXJlcwIrD211dGFibGUtZ2xvYmFscysIc2lnbi1leHQ='
));

CREATE OR REPLACE FUNCTION returns_out_of_bounds LANGUAGE WASM ABI V1 FROM 'test_faulty_abiv1' ARGUMENTS (UInt32) RETURNS Int32;
SELECT returns_out_of_bounds(0 :: UInt32); -- { serverError WASM_ERROR }

CREATE OR REPLACE FUNCTION returns_out_of_bounds2 LANGUAGE WASM ABI V1 FROM 'test_faulty_abiv1' ARGUMENTS (UInt32) RETURNS Int32;
SELECT returns_out_of_bounds2(0 :: UInt32); -- { serverError WASM_ERROR }

-- module itself is ok, test properly defined function from it
CREATE OR REPLACE FUNCTION test_func LANGUAGE WASM ABI V1 FROM 'test_faulty_abiv1' ARGUMENTS (UInt64) RETURNS UInt64 SETTINGS serialization_format = 'CSV';
SELECT test_func(456 :: UInt64), test_func(materialize(521 :: UInt64));

DROP FUNCTION IF EXISTS test_func;
DROP FUNCTION IF EXISTS returns_out_of_bounds;
DROP FUNCTION IF EXISTS returns_out_of_bounds2;
DROP FUNCTION IF EXISTS huge_allocate;
DROP FUNCTION IF EXISTS access_data;
DROP FUNCTION IF EXISTS infinite_loop;
