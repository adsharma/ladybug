describe("CALL subquery (uncorrelated)", function () {
  it("CALL () { RETURN 1 AS x } RETURN x returns one row with x = 1", async function () {
    const queryResult = await conn.query("CALL () { RETURN 1 AS x } RETURN x");
    assert.isTrue(queryResult.hasNext());
    const tuple = await queryResult.getNext();
    assert.equal(tuple["x"], 1);
    assert.isFalse(queryResult.hasNext());
  });

  it("CALL () { RETURN 2 AS a, 'b' AS b } RETURN a, b returns one row", async function () {
    const queryResult = await conn.query(
      "CALL () { RETURN 2 AS a, 'b' AS b } RETURN a, b"
    );
    assert.isTrue(queryResult.hasNext());
    const tuple = await queryResult.getNext();
    assert.equal(tuple["a"], 2);
    assert.equal(tuple["b"], "b");
    assert.isFalse(queryResult.hasNext());
  });
});

describe("CALL subquery (empty inner)", function () {
  it("CALL () { MATCH (n:person) WHERE n.ID < 0 RETURN 1 AS c } RETURN c returns one row with c = null", async function () {
    const queryResult = await conn.query(
      "CALL () { MATCH (n:person) WHERE n.ID < 0 RETURN 1 AS c } RETURN c"
    );
    assert.isTrue(queryResult.hasNext());
    const tuple = await queryResult.getNext();
    assert.isNull(tuple["c"]);
    assert.isFalse(queryResult.hasNext());
  });
});

describe("CALL subquery (correlated)", function () {
  it("MATCH (a:person) WHERE a.ID = 0 CALL (a) { RETURN a.ID AS id } RETURN a.ID, id", async function () {
    const queryResult = await conn.query(
      "MATCH (a:person) WHERE a.ID = 0 CALL (a) { RETURN a.ID AS id } RETURN a.ID, id"
    );
    assert.isTrue(queryResult.hasNext());
    const tuple = await queryResult.getNext();
    assert.equal(tuple["a.ID"], 0);
    assert.equal(tuple["id"], 0);
    assert.isFalse(queryResult.hasNext());
  });

  it("MATCH (a:person) CALL (a) { RETURN 1 AS one } RETURN a.ID, one returns one row per person", async function () {
    const queryResult = await conn.query(
      "MATCH (a:person) CALL (a) { RETURN 1 AS one } RETURN a.ID, one ORDER BY a.ID"
    );
    const expectedIds = [0, 2, 3, 5, 7, 8, 9, 10];
    let i = 0;
    while (queryResult.hasNext()) {
      const tuple = await queryResult.getNext();
      assert.equal(tuple["a.ID"], expectedIds[i]);
      assert.equal(tuple["one"], 1);
      i++;
    }
    assert.equal(i, expectedIds.length);
  });
});
